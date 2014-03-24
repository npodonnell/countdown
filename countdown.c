// Countdown Letters Game Solver 
// Noel P. O'Donnell ,2014


#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define DICT_TEXTFILE "dictionary.txt"
#define MAX_WORD_LENGTH 9
#define REALLOC_FREQ 10000


/*
Source: http://www.math.cornell.edu/~mec/2003-2004/cryptography/subs/frequencies.html
According to this webpage this is the correct ordering of letters according to their
frequency in the English language.
*/
#define ALPHABET "ETAOINSRHDLUCMFYWGPBVKXQJZ"



typedef struct dict_cond{
	unsigned char code;                             // character code
	char* complete_word;                             // if satisfying this condition yields a complete word it's pointer will be stored here, otherwise NULL.
	struct dict_cond *next[MAX_WORD_LENGTH + 1];      // the dict_cond to jump to if our 9 letters cant satisfy this condition. NULL if none left
}dict_cond;


typedef struct dict_entry{
	unsigned char nuletters;                          // number of unique letters in this word
	unsigned char letters[MAX_WORD_LENGTH];           // letters of greatest frequency
	unsigned char occs[MAX_WORD_LENGTH];              // number of occurances of each letter
	struct dict_entry* letter_jps[MAX_WORD_LENGTH];   // jump pointers
	char* complete_word;                              // pointer to complete word
}dict_entry;


int dict_comp(const void* w1, const void* w2){
	/*
	this function is used by the qsort() function to sort words. The words may be read in from the file in any
	order, although typically they will be in forward alphabetical order. Nevertheless they will be resorted in a way that makes
	it faster to find words that fit into the countdown letters' "space". The way they are sorted is first by length, then by the
	frequency of occurances of the letters of the alphabet, starting with the most frequent letter in the language, ending
	with the least frequent.

	Probably not explaining that too well.
	*/

	char* word1 = *(char**)w1;
	char* word2 = *(char**)w2;


	size_t wl1 = strlen(word1);
	size_t wl2 = strlen(word2);

	// see if they sort by length
	if (wl1 > wl2) return -1;
	if (wl1 < wl2) return +1;

	// words are the same length. Now try and sort them
	// according to letter frequency


	unsigned int i = 0; while (i< sizeof(ALPHABET)-1){

		char* cptr; // pointer needed for scanning word1 and word2
		unsigned int occs1 = 0, occs2 = 0;

		// count # of occurances of letter i in word1
		cptr = word1; while ((cptr = strchr(cptr, ALPHABET[i]))) { occs1++; cptr++; if (*cptr == 0) break; }

		// count # of occurances of letter i in word2
		cptr = word2; while ((cptr = strchr(cptr, ALPHABET[i]))) { occs2++; cptr++; if (*cptr == 0) break; }

		if (occs1 > occs2) return -1;
		if (occs1 < occs2) return +1;

		i++;
	}

	// the program should never get here unless there's
	// a duplicate word in the dictionary.


	return 0;
}

unsigned int read_dict(char* filename, char*** dict){

	int rv; // return value of read()
	unsigned int dsize = 0; // # of entries in dictionary
	char wordbuf[MAX_WORD_LENGTH + 1]; // ignore words over this length
	char* wp = wordbuf; // pointer to current write pos in wordbuf

	// Open dictionary file
	int fd = open(filename, O_RDONLY);
	if (fd<0){
		return 0;
	}


	// Rewind the file (not necessary but no harm)
	lseek(0, (off_t)0, SEEK_SET);

	// Read the file character by character
	while (read(fd, wp, 1) == 1){

		if (*wp == '\r' || *wp == '\n'){

			// got a new word.
			*wp = 0; // add a null-terminate


			if (dsize == 0){
				*dict = (char**)malloc(sizeof (char*)* REALLOC_FREQ);
			}
			else if (dsize%REALLOC_FREQ == 0){
				*dict = (char**)realloc(*dict, sizeof (char*)* (dsize + REALLOC_FREQ));
			}

			// allocate & copy character-by-character, changing to uppercase
			(*dict)[dsize] = (char*)malloc((wp - wordbuf) + 1);
			wp = wordbuf; char* cc = (*dict)[dsize]; while (*wp){ *(cc++) = toupper(*(wp++)); } *cc = 0;

			// inc dict size
			dsize++;

			wp = wordbuf;
			printf("Read a word: %s         \r", (*dict)[dsize - 1]);
			while ((rv = read(fd, wp, 1)) == 1 && (*wp == '\r' || *wp == '\n')) { if (rv<1)goto done; } // skip any cr or lf
			wp++;


			continue;
		}

		wp++;


		if (wp - wordbuf > MAX_WORD_LENGTH){
			// word is too long. skip it.
			wp = wordbuf;
			while ((rv = read(fd, wp, 1)) == 1 && *wp != '\r' && *wp != '\n')   { if (rv<1)goto done; } // skip the rest of the word
			while ((rv = read(fd, wp, 1)) == 1 && (*wp == '\r' || *wp == '\n')) { if (rv<1)goto done; } // skip any cr or lf

			// if we're here fd should be pointing to the second character in a new word
			wp++;
		}
	}

done:

	close(fd);

	return dsize;
}

void free_dict(char** dict, unsigned int szdict){
	if (szdict == 0)
		return;

	unsigned int i;

	for (i = 0; i<szdict; i++)
		free(dict[i]);

	free(dict);
}

size_t build_dict(char** dict, dict_entry** pentries, const size_t nwords){
	// this function will return nwords on success otherwise 0

	// allocate a nice big chunk of memory
	int i, j, k, h; dict_entry* entries = (dict_entry*)malloc(sizeof(dict_entry)* nwords);
	*pentries = entries;
	if (entries == (dict_entry*)0){
		return 0;
	}


	// word storing loop
	for (i = 0; i<nwords; i++){

		// count and assign unique letters
		entries[i].complete_word = dict[i];                                  // set pointer to the complete word
		entries[i].nuletters = 0;                                            // set # of unique letters to 0
		for (j = 0; j<sizeof(ALPHABET)-1; j++){                              // scan thru all alphabet letters
			char* cptr = dict[i];                                            // begin character scan
			cptr = strchr(cptr, ALPHABET[j]);                                 // check for an occurance of letter i
			if (!cptr) continue;                                           // letter was not found

			// we have a new unique letter
			unsigned int ulindex = entries[i].nuletters;                     // variable to hold the unique letter index
			entries[i].letters[ulindex] = (*cptr - 65);                        // set the letter (zero-indexed so A=0..Z=25)
			entries[i].occs[ulindex] = 1;                                    // we have one occurance
			entries[i].letter_jps[ulindex] = (i == nwords - 1) ? NULL : entries + i + 1; // set jump pointer to the next word or NULL in this is the last
			entries[i].nuletters++;                                        // inc the number of unique letters
			if (*(++cptr) == 0) continue;                                  // try going on one byte

			// check for additional occurances of this letter
			while ((cptr = strchr(cptr, ALPHABET[j]))) {
				entries[i].occs[ulindex]++;
				if (*(++cptr) == 0) break;
			}
		}
	}


	// jump pointer assignment loop
	for (i = 0; i<nwords; i++){
		for (j = 0; j<entries[i].nuletters; j++){

			unsigned char letter = entries[i].letters[j]; // store this letter. we will look for the next entry where this
			unsigned char occs = entries[i].occs[j];      // letter occurs occs-1 (or less) times.
			for (k = i + 1; k<nwords; k++){
				for (h = 0; h<entries[k].nuletters; h++){
					if (entries[k].letters[h] == letter && entries[k].occs[h]<occs){
						// found an entry for which letter has less occurances.
						entries[i].letter_jps[j] = entries + k;
						goto next_jp;
					}
				}
				// found an entry for which letter has no occurances.
				entries[i].letter_jps[j] = entries + k;
				goto next_jp;
			}
		next_jp:;
		}
	}

	return nwords;
}

struct timespec time_diff(struct timespec start, struct timespec end){

	struct timespec temp;
	if ((end.tv_nsec - start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	}
	else {
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}


inline void query_dict(unsigned char* letters2, dict_entry* entries, size_t nwords){
	dict_entry* ce = entries;
	dict_entry* le = entries + nwords; // Last entry. needed incase the letters satisfy the last word in the dict ('KY')

	/****** this function needs to be as fast as possible, for it is the very crux of the program ******/
	struct timespec requestStart, requestEnd, tt;
	clock_gettime(CLOCK_REALTIME, &requestStart); // get time of start of request


	while (ce){
		int j; for (j = 0; j<ce->nuletters; j++){
			if (letters2[ce->letters[j]] < ce->occs[j]){
				// our letters don't satisfy this word. jump to next word that they might satisfy
				ce = ce->letter_jps[j];
				goto nexti;
			}
		}

		// found a word
		puts(ce->complete_word);
		ce++;

		if (ce == le)
			break; // (usually) the word KY (Which I didn't even know was a word)

	nexti:;
	}

	clock_gettime(CLOCK_REALTIME, &requestEnd); // get time of end of request

	tt = time_diff(requestStart, requestEnd);
	printf("That took %u s %u ms \n", tt.tv_sec, tt.tv_nsec / 1000000);

}

int fix_letters(char* letters){
	letters[MAX_WORD_LENGTH] = 0; // null-terminate
	char* cptr = letters;

	// check all the chars in the string
	while (*cptr != 0){
		*cptr = toupper(*cptr); // convert letter to uppercase
		if (*cptr < 'A' ||
			*cptr > 'Z') return 0; // character didn't fall into the A-Z range
		cptr++;
	}

	if (cptr - letters < MAX_WORD_LENGTH) return 0; // a NULL was found too early
	return 1;
}


int main()
{
	int i, j;
	char ** dict;            // dictionary
	dict_entry* entries;     // lookup table of dictionary entries
	size_t nwords;           // number of words in the dictionary

	// turn off stdin/stdout buffering
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);

	// build the dictionary structure
	nwords = read_dict(DICT_TEXTFILE, &dict);
	if (nwords == 0) goto quit;


	printf("Built a dictionary of size %u\n", (unsigned int)nwords);

	// sort the dictionary
	qsort(dict, nwords, sizeof(char*), dict_comp);

	printf("Sorted words\n");

	// build lookup structure
	if (build_dict(dict, &entries, nwords) != nwords){
		fprintf(stderr, "Error building lookup table\n");
		goto quit;
	}

	printf("Built lookup table\n");

	while (1){
		// Read in letters then convert to uppercase
		char letters[MAX_WORD_LENGTH + 1];
		do{
			printf("Enter %u Letters:", MAX_WORD_LENGTH);
			scanf("%s", letters);
			//if (read(0,letters,MAX_WORD_LENGTH)!=MAX_WORD_LENGTH) continue;
		} while (!fix_letters(letters));


		// convert letters into a string of 26 unsigned chars, each one
		// denoting the number of occurances of that letter. (indexed alphabetically)
		unsigned char letters2[26];
		memset(letters2, '\0', 26);
		for (i = 0; i<MAX_WORD_LENGTH; i++){
			letters2[letters[i] - 65]++;
		}

		// query the dictionary
		query_dict(letters2, entries, nwords);
	}


quit:

	// free the dictionary
	free_dict(dict, nwords);
	if (nwords>0) free(entries);

	return 0;
}




