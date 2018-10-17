#include <string>
#include <fstream>
#include <iostream>

using namespace std;

const char *MAGIC_HEADER = "BPE ";

static int xgetc(unsigned char **in, unsigned char *inl) {
    int     ret;
    if(*in >= inl) return(-1);
    ret = **in;
    (*in)++;
    return(ret);
}

extern "C"
{
	__declspec(dllexport) int yukes_bpe(unsigned char *in, int insz, unsigned char *out, int outsz, int fill_outsz) {
		unsigned char   stack[512 + 4096];
		int             c,
						count,
						i,
						size,
						n;

		unsigned char   *inl,
						*o,
						*outl;

		inl  = in + insz;
		o    = out;
		outl = out + outsz;

		count = 0;
		for(;;) {
			i = 0;
			do {
				if((c = xgetc(&in, inl)) < 0) break;
				if(c > 127) {
					c -= 127;
					while((c > 0) && (i < 256)) {
						stack[i * 2] = i;
						c--;
						i++;
					}
				}
				c++;
				while((c > 0) && (i < 256)) {
					if((n = xgetc(&in, inl)) < 0) break;
					stack[i * 2] = n;
					if(i != n) {
						if((n = xgetc(&in, inl)) < 0) break;
						stack[(i * 2) + 1] = n;
					}
					c--;
					i++;
				}
			} while(i < 256);

			if((n = xgetc(&in, inl)) < 0) break;
			size = n;
			if((n = xgetc(&in, inl)) < 0) break;
			size |= (n << 8);

			while(size || count) {
				if(count) {
					count--;
					n = stack[count + 512];
				} else {
					if((n = xgetc(&in, inl)) < 0) break;
					size--;
				}
				c = stack[n * 2];
				if(n == c) {
					if(o >= outl) return(-1);
					*o++ = n;
				} else {
					if((count + 512 + 2) > sizeof(stack)) return(-1);
					stack[count + 512] = stack[(n * 2) + 1];
					stack[count + 512 + 1] = c;
					count += 2;
				}
			}
		}
		if(fill_outsz) {    // this is what is wanted by the format
			memset(o, 0, outl - o);
			o = outl;
		}
		return(o - out);
	}

	__declspec(dllexport) int bpefile_open_save(char* argv1, char* argv2)
	{
		char *magic = new char[4];
		char *inbuffer;
		char *outbuffer;
		char *readzsize = new char[4];
		char *readsize = new char[4];
		int zsize;
		int size;
		int fillsize;
		
		// comes here if there are three arguments

		// open an input filestream named 'file' in binary mode with the second argument value 'argv[1]'
		ifstream file(argv1, ios::in|ios::binary);

		// checks if the file is open
		if (file.is_open())
		{
			// make sure the starting file position is at the beginning of file
			file.seekg(0, ios::beg);

			// read the contents of the first four bytes into 'magic'
			file.read(magic, 4);

			// use a compare to make sure the first four bytes (char*) of 'magic' match our constant 'MAGIC_HEADER'			
			if (memcmp(magic, MAGIC_HEADER, 4) != 0)
			{
				// if they don't match, insult the user for opening a file that isn't a BPE file
				cout << "Open a BPE file, you numbskull!";

				// return error code
				return -1;
			}
			// continue code since they opened a BPE file

			// set the file position to the 8th byte
			file.seekg(8, ios::beg);

			// read the contents of the next four bytes into 'readzsize'
			file.read(readzsize, 4);

			// set the file position to the 12th byte
			file.seekg(12, ios::beg);

			// read the contents of the next four bytes into 'readsize'
			file.read(readsize, 4);

			// set the value of 'zsize' using the four bytes in 'readzsize'
			memcpy(&zsize, readzsize, sizeof(long));

			// set the value of 'size' using the four bytes in 'readsize'
			memcpy(&size, readsize, sizeof(long));

			// they are only four bytes in size, but we can delete 'readzsize' and 'readsize'
			// and free the HUGE amount of memory they are using :)
			delete[] readzsize;
			delete[] readsize;

			// initialize input buffer 'inbuffer' to the size of 'zsize', our compressed size
			inbuffer = new char[zsize];

			// set the file position to the 16th byte
			file.seekg(16, ios::beg);

			// read the contents of the file of 'zsize' number of bytes into the input buffer 'inbuffer'
			file.read(inbuffer, zsize);

			// calculate the fillsize
			fillsize = size - zsize;

			// the all important closing of the file
			file.close();		
		}	
		else
		{
			// if the file can't open, we of course assume they are an idiot and insult them again
			cout << "\nUnable to open " << argv1 << " you numbskull.  Does the file exist?\n";

			// return error code
			return -1;
		}

		// initialize output buffer 'outbuffer' to the size of 'size', our uncompressed size
		outbuffer = new char[size];

		// call the function which uncompresses the input buffer to the output buffer
		// 'inbuffer' and 'outbuffer' are not unsigned, which the function requires, so we cast them correctly
		// with 'reinterpret_cast<unsigned char*>'
		int x = yukes_bpe((reinterpret_cast<unsigned char*>(inbuffer)), zsize, (reinterpret_cast<unsigned char*>(outbuffer)), size, fillsize);

		// open an output filestream named 'file2' in binary mode with the third argument value 'argv[2]'
		ofstream file2(argv2, ios_base::out|ios_base::binary);

		// checks if the file is open
		if (file2.is_open())
		{
			// if the file opened, write the changed contents of the output buffer 'outbuffer' to disk
			file2.write(outbuffer, size);

			// let the person know the file saved correctly
			cout << "\nSuccessfully saved " << argv2 << " to disk.\n";

			// return successful code
			return 0;
		}
		else
		{
			// let the person know that the file couldn't be saved and they were probably an idiot again
			cout << "\nCannot write " << argv2 << " to disk, you knucklehead.  Do you have the file opened in your hex editor?\n";

			// return an error code
			return -1;
		}
	}
}