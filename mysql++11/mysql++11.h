/*

Modern C++ wrapper for MySQL with simple and convenient usage
Version: 1.0.0.0
Date: 2017 Jan 22

Author: Dao Trung Kien
http://www.mica.edu.vn/perso/kiendt/


Macro Flags:
	STD_OPTIONAL	: using std::optional in C++17 if defined, or std::experimental::optional otherwise

*/




#pragma once


#ifdef _MSC_VER
	#include <winsock.h>
#endif

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include <string>
#include <ctime>
#include <mutex>
#include <vector>
#include <memory>
#include <stdarg.h>

#include "polyfill/function_traits.h"
#include "polyfill/datetime.h"


#ifdef STD_OPTIONAL
	#include <optional>
#else
	#include "polyfill/optional.hpp"
#endif



namespace daotk {

	namespace mysql {

		template <typename Function>
		using function_traits = typename sqlite::utility::function_traits<Function>;

#ifdef STD_OPTIONAL
		template <typename T>
		using optional_type = typename std::optional<T>;
#else
		template <typename T>
		using optional_type = typename std::experimental::optional<T>;
#endif

		class results;
		class connection;


		// iterator class that can be used for iterating returned result rows
		template <typename... Values>
		class result_iterator : std::iterator<std::random_access_iterator_tag, std::tuple<Values...>, int> {
		protected:
			results* res;
			std::shared_ptr<std::tuple<Values...>> data;
			unsigned long long row_index;


			template <int I>
			typename std::enable_if<(I > 0), void>::type
			fetch_impl();

			template <int I>
			typename std::enable_if<(I == 0), void>::type
			fetch_impl();

			void fetch();

			void set_index(unsigned long long i) {
				row_index = i;
				data.reset();
			}

		public:
			result_iterator() {
				res = nullptr;
				row_index = 0;
			}

			result_iterator(results* _res, unsigned long long _row_index)
				: res(_res), row_index(_row_index)
			{ }

			result_iterator(result_iterator& itr) {
				res = itr.res;
				row_index = itr.row_index;
				data = itr.data;
			}

			result_iterator(result_iterator&& itr) {
				res = itr.res;
				row_index = itr.row_index;
				data = std::move(itr.data);
			}

			void operator =(result_iterator& itr) {
				res = itr.res;
				row_index = itr.row_index;
				data = itr.data;
			}

			void operator =(result_iterator&& itr) {
				res = itr.res;
				row_index = itr.row_index;
				data = std::move(itr.data);
			}

			result_iterator& operator ++() {
				set_index(row_index + 1);
				return *this;
			}

			result_iterator operator ++(int) {
				result_iterator tmp(*this);
				set_index(row_index + 1);
				return tmp;
			}

			result_iterator& operator --() {
				set_index(row_index - 1);
				return *this;
			}

			result_iterator operator --(int) {
				result_iterator tmp(*this);
				set_index(row_index - 1);
				return tmp;
			}

			result_iterator& operator +=(int n) {
				set_index(row_index + n);
				return *this;
			}

			result_iterator& operator -=(int n) {
				set_index(row_index - n);
				return *this;
			}

			result_iterator operator +(int n) {
				return result_iterator(res, row_index + n);
			}

			result_iterator operator -(int n) {
				return result_iterator(res, row_index - n);
			}

			bool operator == (const result_iterator& itr) const {
				return row_index == itr.row_index;
			}
			bool operator != (const result_iterator& itr) const {
				return row_index != itr.row_index;
			}
			bool operator > (const result_iterator& itr) const {
				return row_index > itr.row_index;
			}
			bool operator < (const result_iterator& itr) const {
				return row_index < itr.row_index;
			}
			bool operator >= (const result_iterator& itr) const {
				return row_index >= itr.row_index;
			}
			bool operator <= (const result_iterator& itr) const {
				return row_index <= itr.row_index;
			}

			std::tuple<Values...>& operator *() {
				if (!data) fetch();
				return *data;
			}

			std::tuple<Values...>* operator ->() {
				if (!data) fetch();
				return data.get();
			}
		};


		// container-like result class
		template <typename... Values>
		class result_containter {

		public:
			using tuple_type = std::tuple<Values...>;

			friend class results;

		protected:
			results* res;

			result_containter(results* _res)
				: res(_res)
			{}

		public:
			result_iterator<Values...> begin();
			result_iterator<Values...> end();
		};



		// result fetching
		class results {

			friend class connection;

		protected:
			MYSQL_RES* res;
			MYSQL* my_conn;
			MYSQL_ROW row = nullptr;
			bool started = false;

			results(MYSQL* _my_conn, MYSQL_RES* _res)
				: my_conn(_my_conn), res(_res)
			{ }


			template <typename Function, std::size_t Index>
			using nth_argument_type = typename function_traits<Function>::template argument<Index>;

			template <typename Function, typename... Values>
			typename std::enable_if<(sizeof...(Values) < function_traits<Function>::arity), bool>::type
			bind_and_call(Function&& callback, Values&&... values) {
				nth_argument_type<Function, sizeof...(Values)> value;
				get_value(sizeof...(Values), value);

				return bind_and_call(callback, std::forward<Values&&>(values)..., std::move(value));
			}

			template <typename Function, typename... Values>
			typename std::enable_if<(sizeof...(Values) == function_traits<Function>::arity), bool>::type
			bind_and_call(Function&& callback, Values&&... values) {
				return callback(std::forward<Values&&>(values)...);
			}

		public:
			results(const results& r) = delete;
			void operator =(const results& r) = delete;

			results()
				: my_conn(nullptr), res(nullptr)
			{ }

			results(results&& r) {
				my_conn = r.my_conn;
				res = r.res;

				r.my_conn = nullptr;
				r.res = nullptr;
			}

			void operator = (results&& r) {
				if (res != nullptr) mysql_free_result(res);

				my_conn = r.my_conn;
				res = r.res;
				row = r.row;
				started = r.started;

				r.my_conn = nullptr;
				r.res = nullptr;
				r.row = nullptr;
				r.started = false;
			}

			virtual ~results() {
				if (res != nullptr) mysql_free_result(res);
			}

			// return number of rows
			unsigned long long count() {
				return res == nullptr ? 0 : mysql_num_rows(res);
			}

			// return number of fields
			unsigned int fields() {
				return res == nullptr ? 0 : mysql_num_fields(res);
			}

			// return true if query was executed successfully
			operator bool() const {
				return my_conn != nullptr;
			}

			// return true if no data was returned
			bool is_empty() {
				return count() == 0;
			}

			// return true if passed the last row
			bool eof() {
				if (res == nullptr) return true;
				if (!started) reset();
				return row == nullptr;
			}

			// go to first row and fetch data
			bool reset() {
				return seek(0);
			}

			// go to nth row and fetch data, return true if successful
			bool seek(unsigned long long n) {
				if (res == nullptr) return false;
				mysql_data_seek(res, n);
				row = mysql_fetch_row(res);
				started = true;
				return row != nullptr;
			}

			// go to next row and fetch data
			bool next() {
				row = mysql_fetch_row(res);
				return row != nullptr;
			}

			// return curent row index
			unsigned long long tell() const {
				return (unsigned long long)mysql_row_tell(res);
			}

			// iterate through all rows, each time execute the callback function
			template <typename Function>
			int each(Function callback) {
				if (my_conn == nullptr) return -1;
				if (res == nullptr) return 0;

				reset();

				int count = 0;
				while (row != nullptr) {
					count++;
					if (!bind_and_call(callback)) break;
					next();
				}

				return count;
			}

			// create an object that can be used to iterate through the rows like STL containers
			template <typename... Values>
			result_containter<Values...> as_container() {
				return result_containter<Values...>{this};
			}




			// get-value functions in different ways and types...

			bool get_value(int i, bool& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = (std::stoi(row[i]) != 0);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, int& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stoi(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, unsigned int& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stoul(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, long& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stol(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, unsigned long& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stoul(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, long long& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stoll(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, unsigned long long& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stoull(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, float& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stof(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, double& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stod(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, long double& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				try {
					value = std::stold(row[i]);
					return true;
				}
				catch (std::exception) {
					return false;
				}
			}

			bool get_value(int i, std::string& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				value = row[i];
				return true;
			}

			bool get_value(int i, datetime& value) {
				if (!started) reset();
				if (row == nullptr || row[i] == nullptr) return false;

				value.from_sql(row[i]);
				return true;
			}

			template <typename Value>
			bool get_value(Value& value) {
				return get_value(0, value);
			}

			template <typename Value>
			Value get_value(int i = 0) {
				Value v;
				get_value(i, v);
				return std::move(v);
			}

			template <typename Value>
			void get_value(int i, optional_type<Value>& value) {
				Value v;
				if (get_value(i, v)) value = v;
			}

			template <typename Value>
			void get_value(optional_type<Value>& value) {
				get_value(0, value);
			}

			template <typename Value>
			Value operator[](int i) {
				Value v;
				get_value(i, v);
				return std::move(v);
			}


		protected:
			template <typename Value>
			void fetch_impl(int i, Value& value) {
				get_value(i, value);
			}
			
			template <typename Value, typename... Values>
			void fetch_impl(int i, Value& value, Values&... values) {
				get_value(i, value);
				fetch_impl(i+1, std::forward<Values&>(values)...);
			}

		public:
			// get data from every fields of the current row
			template <typename... Values>
			bool fetch(Values&... values) {
				if (!started) reset();
				if (row == nullptr) return false;

				fetch_impl(0, std::forward<Values&>(values)...);
				return true;
			}
		};



		// database connection and query...
		class connection : public std::enable_shared_from_this<connection> {
		protected:
			MYSQL* my_conn;
			std::mutex mutex;

		public:
			// open a connection (close the old one if already open), return true if successful
			bool open(std::string server, std::string username, std::string password, std::string dbname = "", unsigned int timeout = 0) {
				if (is_open()) close();

				std::lock_guard<std::mutex> mg(mutex);

				my_conn = mysql_init(nullptr);
				if (my_conn == nullptr) return false;

				if (timeout > 0) mysql_options(my_conn, MYSQL_OPT_CONNECT_TIMEOUT, (char*)&timeout);

				if (nullptr == mysql_real_connect(my_conn, server.c_str(), username.c_str(), password.c_str(), dbname.c_str(), 0, NULL, 0)) {
					mysql_close(my_conn);
					my_conn = nullptr;
					return false;
				}

				return true;
			}

			connection()
				: my_conn(nullptr)
			{ }

			connection(std::string server, std::string username, std::string password, std::string dbname = "", unsigned int timeout = 0)
				: my_conn(nullptr)
			{
				open(server, username, password, dbname, timeout);
			}

			void close() {
				std::lock_guard<std::mutex> mg(mutex);

				if (my_conn != nullptr) {
					mysql_close(my_conn);
					my_conn = nullptr;
				}
			}

			virtual ~connection() {
				close();
			}

			// same as is_open()
			operator bool() const {
				return my_conn != nullptr;
			}

			bool is_open() const {
				return my_conn != nullptr;
			}

			// raw MySQL connection in case needed
			operator MYSQL*() {
				return my_conn;
			}


			// wrapping of some functions

			unsigned long long last_insert_id() const {
				return mysql_insert_id(my_conn);
			}

			unsigned int error_code() const {
				return mysql_errno(my_conn);
			}

			const char* error_message() const {
				return mysql_error(my_conn);
			}

		protected:
			results query_impl2(const char* fmt_str, va_list args) {
				size_t size = 256;
				std::vector<char> buf(size);

				while (true) {
					int needed = std::vsnprintf(&buf[0], size, fmt_str, args);

					if (needed <= (int)size && needed >= 0)
						return query(&buf[0]);

					size = (needed > 0) ? (needed + 1) : (size * 2);
					buf.resize(size);
				}
			}

			results query_impl1(const char* fmt_str,...) {
				va_list vargs;
				va_start(vargs, fmt_str);
				results res = query_impl2(fmt_str, vargs);
				va_end(vargs);
				return std::move(res);
			}

		public:
			// execute query given by string
			results query(const std::string& query_str) {
				std::lock_guard<std::mutex> mg(mutex);

				int ret = mysql_real_query(my_conn, query_str.c_str(), query_str.length());
				if (ret != 0) return results{};

				ret = mysql_errno(my_conn);
				if (ret == CR_SERVER_GONE_ERROR || ret == CR_SERVER_LOST) return results{};

				return results{ my_conn, mysql_store_result(my_conn) };
			}

			// execute query with printf-style substitutions
			template <typename... Values>
			results query(const std::string& fmt_str, Values... values) {
				return query_impl1(fmt_str.c_str(), std::forward<Values>(values)...);
			}
		};






		template <typename... Values>
		template <int I>
		typename std::enable_if<(I > 0), void>::type
		result_iterator<Values...>::fetch_impl() {
			res->get_value(I, std::get<I>(*data));
			fetch_impl<I - 1>();
		}

		template <typename... Values>
		template <int I>
		typename std::enable_if<(I == 0), void>::type
		result_iterator<Values...>::fetch_impl() {
			res->get_value(I, std::get<I>(*data));
		}

		template <typename... Values>
		void result_iterator<Values...>::fetch() {
			res->seek(row_index);
			data = std::make_shared<std::tuple<Values...>>();
			fetch_impl<sizeof...(Values) - 1>();
		}


		template <typename... Values>
		result_iterator<Values...> result_containter<Values...>::begin() {
			return result_iterator<Values...>{res, 0};
		}

		template <typename... Values>
		result_iterator<Values...> result_containter<Values...>::end() {
			return result_iterator<Values...>{res, res->count()};
		}
	}
}
