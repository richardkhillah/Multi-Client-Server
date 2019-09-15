#include "FileManager.h"

File::File(std::string filename, std::ios::openmode mode)
: _filename(filename), _mode(mode)
{	
	// if no mode is supplied, do not open fstream object

	// if mode is supplied, attempt to open a fstream object,
	// set ios_base::openmode _iostate = _fs.fdstate();

	// if any exceptions were caught while attempting open _fstream
		// dig into _iostate.<flag>() options, and handle accordingly
			// digging into C errno if required.

	// return control to caller
}

// void File::open(std::ios::openmode mode)
File& File::open()
{
	if( !_fs.is_open() ) {
		_fs.open(_filename, _mode);
	}

	if( !_fs.is_open() ) {
		// throw error
		perror("ERROR: ");
		exit(-1);
	}
	return *this;
}

File& File::write(const char* buf, size_t nbytes)
{
	if( !_fs.is_open() ) {
		_fs.open(_filename, _mode|APPEND_ONLY);

	}
	_fs.write(buf, nbytes);
	
	// updateStreamMetaData();
	//TODO: proper error checking
	if( _fs.bad() ) {
		std::cerr << "Bad Bit Set\n";
	}
	return *this;
}

File& File::read(char* buf, size_t nbytes)
{
	if( !_fs.is_open() ) {
		// TODO: check _fs permissions, i.e. wheter _fs is open in read mode
		_fs.open(_filename, _mode|READ_ONLY);
	}

	_fs.read(buf, nbytes);

	// int bytesRead = _fs.gcount();

	return *this;
}

File& File::truncate(bool leaveOpen) {
	if( _fs.is_open() ) {
		_fs.close();
	}
	remove(_filename.c_str());
	_fs.open(_filename, _mode);

	if( !leaveOpen ) {
		_fs.close();
	}
	return *this;
}

File& File::close()
{
	_fs.close();
	return *this;
}

#ifdef TEST
const char* testCaseSeparator = "\n\n================================================================================\n";

const char* buf1 = "Hello, world!";
const char* buf2 = "Goodbye, world...";
const char* buf3 = "This is one of those times I'd really like you to get outta my head.";
const char* buf4 = "Yea.. so now that I'm out of my head I'm out of my mind... Oh the irony";

void testWrite() {
	std::cerr << testCaseSeparator;
	std::cerr << "Testing method \"testWrite()\"\n";
	File f("test", WRITE_ONLY);
	f.open();
	f.write(buf1, strlen(buf1));
	f.close();

	File f2("test2.txt", WRITE_ONLY);
	f2.open();
	f2.write(buf2, strlen(buf2));
	f2.close();
}

void testRead() {
	std::cerr << testCaseSeparator;
	std::cerr << "Testing Method \"testRead()\"\n";
	char buf[strlen(buf2)+1];
	bzero(buf, strlen(buf2)+1);

	File f3("test2.txt", READ_ONLY);

	f3.open();
	f3.read(buf, strlen(buf2));
	std::cout << "test2.txt contents: " << buf;
	f3.close();
}

void testTruncate() {
	std::cerr << testCaseSeparator;
	std::cout << "testing method \"testTruncate()\" with 'KEEP_OPEN' option...\n";
	File f("test3.txt", WRITE_ONLY);

	std::cerr << "Writing: \"" << buf4 << "\" to \"test3.txt\"...\n";
	f.write(buf4, strlen(buf4));
	f.close();

	int len = std::max(strlen(buf3), strlen(buf4));
	char buf[len+1];

	bzero(buf, len+1);
	f.read(buf, strlen(buf4));
	std::cerr << "Contents of \"test3.txt\": " << buf << std::endl;
	std::cerr << "Truncating \"test3.txt\"...";
	f.truncate(KEEP_OPEN);
	std::cout << "done\n";

	bzero(buf, len+1);
	f.read(buf, strlen(buf4));
	f.close();
	std::cerr << "Contents of \"test3.txt\": " << buf << std::endl;


	std::cerr << "Testing File::truncate(KEEP_OPEN)...";
	File f2("test4.txt", WRITE_ONLY);
	std::cerr << "Writing: \"" << buf3 << "\" to \"test4.txt\"...\n";
	f2.write(buf3, strlen(buf3));
	f2.close();

	bzero(buf, len+1);
	f2.read(buf, len);
	std::cerr << "Contents of \"test4.txt\": " << buf << std::endl;
	std::cerr << "Truncating \"test4.txt\"...";
	f2.truncate(KEEP_OPEN);
	std::cout << "done\n";
	f2.write("ERROR", strlen("ERROR"));

	bzero(buf, len+1);
	f2.close();
	f2.read(buf, len);
	std::cerr << "Contents of \"test4.txt\": " << buf << std::endl;
}

void testMultiObjectmethodAccessor() {
	std::cerr << testCaseSeparator;
	std::cerr << "Testing \"testMultiObjectmethodAccessor()\"\n";
	std::cerr << "...calling File f(\"test5.txt\", WRITE_ONLY); f.write(buf3, strlen(buf3)).close().read(buf, strlen(buf3));\n";

	char buf[strlen(buf3)+1];
	bzero(buf, strlen(buf3)+1);

	File f("test5.txt", WRITE_ONLY);
	f.write(buf3, strlen(buf3)).close().read(buf, strlen(buf3));
	std::cerr << "contents of \"test5.txt\": " << buf << std::endl;
}

void testeof() {
	
}

int main(void)
{
	testWrite();

	testRead();

	testTruncate();

	testMultiObjectmethodAccessor();

	return 0;
}
#endif