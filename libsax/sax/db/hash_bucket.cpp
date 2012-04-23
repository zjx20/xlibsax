
#include <hash_bucket.h>
namespace sax{
	
	bool bucket_chain::write(FILE* f) {
		if (!isDirty) {
			return false;
		}

		if (!isLoaded) {
			return false;
		}

		if (!f) {
			return false;
		}

		share_rwlock lock(filelock_, 1);
		
		if ( 0 != fseek(f, fpos, SEEK_SET) )
		{
			return false;
		}

		if (1 != fwrite(&num, sizeof(int), 1, f) ) {
			return false;
		}

		size_t blockSize = bucketSize_ - sizeof(int) - sizeof(long);

		if (1 != fwrite(str, blockSize, 1, f) ) {
			return false;
		}

		if (next)
			nextfpos = next->fpos;

		if (1 != fwrite(&nextfpos, sizeof(long), 1, f) ) {
			return false;
		}

		isDirty = false;
		isLoaded = true;

		return true;
	}
	
	bool bucket_chain::read_file(FILE *f){
		if (!f) {
			return false;
		}

		share_rwlock lock(filelock_, 1);

		if (isLoaded) {
			return true;
		}

		if ( 0 != fseek(f, fpos, SEEK_SET) )
			return false;

		if (1 != fread(&num, sizeof(int), 1, f) ) {
			return false;
		}

		size_t blockSize = bucketSize_ - sizeof(int) - sizeof(long);

		if ( !str) {
			str = new char[blockSize];
		}

		if (1 != fread(str, blockSize, 1, f) ) {
			return false;
		}

		if (1 != fread(&nextfpos, sizeof(long), 1, f) ) {
			return false;
		}

		if (nextfpos !=0) {
			if ( !next)
				next = new bucket_chain(bucketSize_, filelock_);
			next->fpos = nextfpos;
		}

		isLoaded = true;
		isDirty = false;

		return true;
	}
}
