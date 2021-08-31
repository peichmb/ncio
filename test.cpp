#include "ncio.h"
#include "barray.h"


void print_chunk(const BMatrix<float>& chunk) {
	for (int i=0; i<chunk.get_nrows(); i++) {
		for (int j=0; j<chunk.get_ncols(); j++) {
			std::cout << chunk(i,j) << ( j==chunk.get_ncols()-1 ? "\n" : " ");
		}
	}
}

int main() {

	std::cout << "Writing test file...\n";
	int nrows = 3, ncols = 5;
	BMatrix<float> chw(nrows,ncols); // Chunk I wanna modify and write to file
	bool usealloc = true;

	if (usealloc) {
		// Buffer of 7 lines will be filled up before being dumped to file automatically
		NCFile_float_w* output_file = new NCFile_float_w
			("test_write.nc", "varxy", "x", "y", ncols, 7);

		for (int step=1; step<5; step++) {
			// Each step writes 3 lines to the buffer/file
			for (int i=0; i<chw.get_nrows(); i++) {
				for (int j=0; j<chw.get_ncols(); j++) {
					chw(i,j) = step;
				}
			}
			// Write this step to file
			output_file->write_chunk(chw);
		}

		// Destructor dumps buffer and closes file
		// Failing to delete means last buffer won't be written to file!
		delete output_file;
	} else {
		// Buffer of 7 lines will be filled up before being dumped to file automatically
		NCFile_float_w output_file("test_write.nc", "varxy", "x", "y", ncols, 7);

		for (int step=1; step<5; step++) {
			// Each step writes 3 lines to the buffer/file
			for (int i=0; i<chw.get_nrows(); i++) {
				for (int j=0; j<chw.get_ncols(); j++) {
					chw(i,j) = step;
				}
			}
			// Write this step to file
			output_file.write_chunk(chw);
		}
	}


	// Read the test file. The array I read it into has 5 lines, the read buffer has 14 lines.
	// Since I only wrote 12 lines to the file, the whole file will be loaded to the buffer,
	// 	and chunks of 5 lines will be returned every time read_chunk is called.
	// 	When there are no more lines to return, the function returns the fill value 32767.
	std::cout << "Reading test file...\n";

	BMatrix<float> chw2(5,ncols);
	NCFile_float_r ofr("test_write.nc", "varxy", "x", "y", 14);

	ofr.read_chunk(chw2);
	print_chunk(chw2);
	std::cout << "\n";

	ofr.read_chunk(chw2);
	print_chunk(chw2);
	std::cout << "\n";

	ofr.read_chunk(chw2);
	print_chunk(chw2);
	std::cout << "\n";

	ofr.read_chunk(chw2);
	print_chunk(chw2);
	std::cout << "\n";

}
