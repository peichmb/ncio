#ifndef NCIO_H
#define NCIO_H

#include "./ncio_core.h"

class NCFile_float_r : public NCFile_r<float> {

private:

	int read_from_nc_file(size_t start[2], size_t count[2]) {
		int ret_val = nc_get_vara_float(get_file_id(), get_var_id(), start, count,
				&get_current_pos_buffer());
		return ret_val;
	}

public:

	NCFile_float_r(std::string file_name, std::string var_name, std::string dim_rows_name,
		       std::string dim_cols_name, size_t nrows_buffer)
		     : NCFile_r<float>(file_name, var_name, dim_rows_name, dim_cols_name,
		       		       nrows_buffer) {

		set_type(NC_FLOAT);
		int ret_val = initialize();
	}

	~NCFile_float_r() {
		close();
	}
};


class NCFile_float_w : public NCFile_w<float> {

private:

	int write_to_nc_file(size_t start[2], size_t count[2]) {
		int ret_val = nc_put_vara_float(get_file_id(), get_var_id(), start, count,
				&get_start_pos_buffer());
		return ret_val;
	}

public:

	NCFile_float_w(std::string file_name, std::string var_name, std::string dim_rows_name,
		       std::string dim_cols_name, size_t ncols, size_t nrows_buffer)
		     : NCFile_w<float>(file_name, var_name, dim_rows_name, dim_cols_name,
		       		       ncols, nrows_buffer) {

		set_type(NC_FLOAT);
		int ret_val = initialize();
	}

	~NCFile_float_w() {
		close();
	}
};


class NCFile_double_r : public NCFile_r<double> {

private:

	int read_from_nc_file(size_t start[2], size_t count[2]) {
		int ret_val = nc_get_vara_double(get_file_id(), get_var_id(), start, count,
				&get_current_pos_buffer());
		return ret_val;
	}

public:

	NCFile_double_r(std::string file_name, std::string var_name, std::string dim_rows_name,
		       std::string dim_cols_name, size_t nrows_buffer)
		     : NCFile_r<double>(file_name, var_name, dim_rows_name, dim_cols_name,
		       		       nrows_buffer) {

		set_type(NC_DOUBLE);
		int ret_val = initialize();
	}

	~NCFile_double_r() {
		close();
	}
};


class NCFile_double_w : public NCFile_w<double> {

private:

	int write_to_nc_file(size_t start[2], size_t count[2]) {
		int ret_val = nc_put_vara_double(get_file_id(), get_var_id(), start, count,
				&get_start_pos_buffer());
		return ret_val;
	}

public:

	NCFile_double_w(std::string file_name, std::string var_name, std::string dim_rows_name,
		       std::string dim_cols_name, size_t ncols, size_t nrows_buffer)
		     : NCFile_w<double>(file_name, var_name, dim_rows_name, dim_cols_name,
		       		       ncols, nrows_buffer) {

		set_type(NC_DOUBLE);
		int ret_val = initialize();
	}

	~NCFile_double_w() {
		close();
	}
};


#endif // NCIO_H
