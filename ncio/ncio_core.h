#ifndef NCIO_CORE_H
#define NCIO_CORE_H

#include <netcdf.h>
#include <string>
#include <iostream>
#include <assert.h>
#include <algorithm>
#include "barray.h"

namespace {

	const int END_OF_FILE = 1;
	const int BUFFER_TOO_BIG = 2;
	const int READ_MODE_FILE = 3;
	const int WRITE_MODE_FILE = 4;


	void throw_error(int error_code, std::string file_name) {

		if (error_code == 0) {
			return;
		}
		if (error_code < 0) {
			std::cout << nc_strerror(error_code) << "\n";
		} else {

			switch (error_code) {
			case END_OF_FILE:
				std::cout << "End of file reached.\n";
			break;
			case BUFFER_TOO_BIG:
				std::cout << "Resizing buffer to match number of rows in netCDF file.\n";
			break;
			case WRITE_MODE_FILE:
				std::cout << "Can't read file; it's open in write mode.\n";
			break;
			case READ_MODE_FILE:
				std::cout << "Can't write file; it's open in read mode.\n";
			break;
			default:
				std::cout << "Undefined error\n";
			break;
			}
		}
		std::cout << "File: " << file_name << std::endl;
	}

}


// Abstract base class for netCDF files
class NCFile {

protected:

	std::string _file_name;
	std::string _var_name;
	std::string _dim_rows_name;
	std::string _dim_cols_name;

	bool _open;
	int _mode;
	int _file_id;
	int _dim_rows_id;
	int _dim_cols_id;
	int _var_id;
	int _nc_type;
	size_t _nrows;
	size_t _ncols;
	size_t _current_row;

	virtual int _init() = 0;

	virtual int initialize() = 0;

	NCFile(std::string file_name, std::string var_name, std::string dim_rows_name, std::string dim_cols_name) {
		
		_file_name = file_name;
		_var_name = var_name;
		_dim_rows_name = dim_rows_name;
		_dim_cols_name = dim_cols_name;
		_current_row = 0;

	}


	~NCFile() {};

	virtual int close() = 0;

};


// Abstract base class for readable files
// --------------------------------------
template <typename T>
class NCFile_r : private NCFile {

private:

	const T _padding_value = (T)32767;

	BMatrix<T>* _buffer;
	size_t _nrows_buffer;
	size_t _current_row_buffer;

	int _init();

	int _load_buffer();

	void _pad_chunk(BMatrix<T>& chunk, size_t first_row);

protected:

	void set_type(int nc_type) { _nc_type = nc_type; };

	int get_file_id() { return _file_id; }

	int get_var_id() { return _var_id; }

	T& get_current_pos_buffer() { return (*_buffer)(_current_row_buffer, 0); }

	T& get_start_pos_buffer() { return (*_buffer)(0, 0); }

	virtual int read_from_nc_file(size_t start[2], size_t count[2]) = 0;

	int initialize();

	NCFile_r(std::string file_name, std::string var_name, std::string dim_rows_name,
		 std::string dim_cols_name, size_t nrows_buffer) :
		 NCFile(file_name, var_name, dim_rows_name, dim_cols_name) {

		_mode = NC_NOWRITE;
		_nrows_buffer = nrows_buffer;
		_current_row_buffer = 0;
		throw_error(_init(), file_name);
	}
	
	~NCFile_r() {}
	
public:

	void read_chunk(BMatrix<T>& chunk);

	void write_chunk(BMatrix<T>& chunk) { return throw_error(READ_MODE_FILE, _file_name); }

	int close();
};


template <typename T>
int NCFile_r<T>::_init() {

	int ret_val;


	ret_val = nc_open(_file_name.c_str(), _mode, &_file_id);
	throw_error(ret_val, _file_name);

	ret_val = nc_inq_varid(_file_id, _var_name.c_str(), &_var_id);
	throw_error(ret_val, _file_name);

	ret_val = nc_inq_dimid(_file_id, _dim_rows_name.c_str(), &_dim_rows_id);
	throw_error(ret_val, _file_name);

	ret_val = nc_inq_dimlen(_file_id, _dim_rows_id, &_nrows);
	throw_error(ret_val, _file_name);

	ret_val = nc_inq_dimid(_file_id, _dim_cols_name.c_str(), &_dim_cols_id);
	throw_error(ret_val, _file_name);

	ret_val = nc_inq_dimlen(_file_id, _dim_cols_id, &_ncols);
	throw_error(ret_val, _file_name);
	
	// Initialize buffer
	if (_nrows_buffer > _nrows) {
		throw_error(BUFFER_TOO_BIG, _file_name);
		_nrows_buffer = _nrows;
	}
	_buffer = new BMatrix<T>(_nrows_buffer, _ncols);

	if (!ret_val) {
		_open = true;
	}

	return ret_val;
}


template <typename T>
void NCFile_r<T>::_pad_chunk(BMatrix<T>& chunk, size_t first_row) {

	size_t nrows_padding = chunk.get_nrows() - first_row;
	T* chunk_ptr = &chunk(first_row,0);

	for (size_t i=0; i<nrows_padding; i++) {
		for (size_t j=0; j<_ncols; j++) {
			*chunk_ptr = _padding_value;
			*chunk_ptr++;
		}
	}
}


template <typename T>
int NCFile_r<T>::initialize () {

	return _load_buffer();
}


template <typename T>
int NCFile_r<T>::_load_buffer() {

	size_t start[2], count[2];
	start[0] = _current_row;
	start[1] = 0;
	if (_current_row + _nrows_buffer > _nrows) {
		count[0] = _nrows - _current_row;
	} else {
		count[0] = _nrows_buffer;
	}
	count[1] = _ncols;
	_current_row_buffer = 0;

	if (count[0] < _nrows_buffer) {
		throw_error(END_OF_FILE, _file_name);
	}

	int ret_val = read_from_nc_file(start, count);

	return ret_val;
}


template <typename T>
void NCFile_r<T>::read_chunk(BMatrix<T>& chunk) {

	assert(chunk.get_ncols() == _ncols);

	size_t last_row = std::min(_current_row + (size_t)chunk.get_nrows(), _nrows);
	size_t nrows_padding = chunk.get_nrows() - (last_row - _current_row);
	T* buffer_ptr = &get_current_pos_buffer();
	T* chunk_ptr = &chunk(0,0);

	if (_current_row >= last_row) {
		throw_error(END_OF_FILE, _file_name);
		_pad_chunk(chunk, 0);
	}

	while (_current_row < last_row) {
		
		for (int j=0; j<_ncols; j++) {
			*chunk_ptr = *buffer_ptr;
			*chunk_ptr++;
			*buffer_ptr++;
		}
		_current_row++;
		_current_row_buffer++;

		if (_current_row_buffer == _nrows_buffer) {
			throw_error(_load_buffer(), _file_name);
			buffer_ptr = &get_start_pos_buffer();
		}
	}

	if (nrows_padding > 0) {
		size_t first_row_padding = chunk.get_nrows() - nrows_padding;
		_pad_chunk(chunk, first_row_padding);
	}
}


template <typename T>
int NCFile_r<T>::close() {
	int ret_val;
	if (_open) {
		ret_val = nc_close(_file_id);
		if (!ret_val) {
			_open = false;
		} else {
			throw_error(ret_val, _file_name);
		}
	}

	std::cout << "deleting read buffer..." << std::endl;
	delete _buffer;
	return ret_val;
}


// Abstract base class for writable files
// --------------------------------------
template <typename T>
class NCFile_w : private NCFile {

private:

	BMatrix<T>* _buffer;
	size_t _nrows_buffer;
	size_t _current_row_buffer;

	int _init();

	int dump_buffer();

protected:

	void set_type(int nc_type) { _nc_type = nc_type; }

	int get_file_id() { return _file_id; }

	int get_var_id() { return _var_id; }

	T& get_current_pos_buffer() { return (*_buffer)(_current_row_buffer, 0); }

	T& get_start_pos_buffer() { return (*_buffer)(0, 0); }

	int initialize();

	virtual int write_to_nc_file(size_t start[2], size_t count[2]) = 0;
	
	NCFile_w(std::string file_name, std::string var_name, std::string dim_rows_name,
	       std::string dim_cols_name, size_t ncols, size_t nrows_buffer)
	     : NCFile(file_name, var_name, dim_rows_name, dim_cols_name) {

		_mode = NC_WRITE;
		_ncols = ncols;
		_nrows_buffer = nrows_buffer;
		_current_row_buffer = 0;
		throw_error(_init(), _file_name);
	}

	~NCFile_w() {}
	
public:
	
	void read_chunk(BMatrix<T>& chunk) { return throw_error(WRITE_MODE_FILE, _file_name); }

	void write_chunk(BMatrix<T>& chunk);

	int close();
};


template <typename T>
int NCFile_w<T>::_init() {

	int ret_val;

	ret_val = nc_create(_file_name.c_str(), NC_NETCDF4, &_file_id);
	throw_error(ret_val, _file_name);

	ret_val = nc_def_dim(_file_id, _dim_rows_name.c_str(), NC_UNLIMITED, &_dim_rows_id);
	throw_error(ret_val, _file_name);

	ret_val = nc_def_dim(_file_id, _dim_cols_name.c_str(), _ncols, &_dim_cols_id);
	throw_error(ret_val, _file_name);

	_buffer = new BMatrix<T>(_nrows_buffer, _ncols);

	return ret_val;
}


template <typename T>
int NCFile_w<T>::initialize () {

	int ret_val;
	int dim_ids[2];
	dim_ids[0] = _dim_rows_id;
	dim_ids[1] = _dim_cols_id;
	ret_val = nc_def_var(_file_id, _var_name.c_str(), _nc_type, 2, dim_ids, &_var_id);
	throw_error(ret_val, _file_name);

	ret_val = nc_enddef(_file_id);
	throw_error(ret_val, _file_name);

	if (!ret_val) {
		_open = true;
	}

	return ret_val;
}


template <typename T>
int NCFile_w<T>::dump_buffer() {

	size_t start[2];
	size_t count[2];

	start[0] = _current_row;
	start[1] = 0;
	count[0] = _current_row_buffer;
	count[1] = _ncols;

	int ret_val = write_to_nc_file(start, count);
	throw_error(ret_val, _file_name);

	_current_row += _current_row_buffer;
	_current_row_buffer = 0;

	return ret_val;
}


template <typename T>
void NCFile_w<T>::write_chunk(BMatrix<T>& chunk) {

	assert(chunk.get_ncols() == _ncols);

	size_t current_row_chunk = 0;
	T* chunk_ptr = &chunk(0,0);
	T* buffer_ptr = &get_current_pos_buffer();

	while (current_row_chunk < chunk.get_nrows()) {
		
		for (int j=0; j<_ncols; j++) {
			*buffer_ptr = *chunk_ptr;
			*chunk_ptr++;
			*buffer_ptr++;
		}

		_current_row_buffer++;
		current_row_chunk++;

		if (_current_row_buffer == _nrows_buffer) {
			dump_buffer();
			buffer_ptr = &get_start_pos_buffer();
		}
	}
}


template <typename T>
int NCFile_w<T>::close() {

	int ret_val;

	if (_open) {
		dump_buffer();
		ret_val = nc_close(_file_id);
		if (!ret_val) {
			_open = false;
		} else {
			throw_error(ret_val, _file_name);
		}
	}

	std::cout << "deleting write buffer..." << std::endl;
	delete _buffer;
	return ret_val;
}


#endif // NCIO_CORE_H
