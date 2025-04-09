# mysql+++

## Description:
This is a lightweight wrapper for MySQL with simple and convenient usage in Modern C++ (C++11 or later).

## License:
Completely free. No restriction!

## Requirements:
- C++11 ready compiler - please check if your compiler is up to date (at the moment: MSVC 2015, g++ 5.1, clang 3.8)
- MySQL Connector/C:
	+ For Windows: https://dev.mysql.com/downloads/connector/c/
	+ For Linux: `sudo apt-get install libmysqlclient-dev`
- No .cpp, no compiled library, no binary needed

## Credits:
- `sqlite::utility::function_traits` is from @github/aminroosta/sqlite_modern_cpp: https://github.com/aminroosta/sqlite_modern_cpp
- `std::experimental::optional` is from @github/akrzemi1/Optional: https://github.com/akrzemi1/Optional

## How to use (by examples):
Note that in the following examples, a table defined as follows is used:
```sql
		create table person(
			id int unsigned auto_increment primary key,
			name varchar(50),
			weight double,
			birthday date,
			avatar int
		);

		insert into person(name, weight, birthday, avatar) values
			("Nguyen Van X", 62.54, "1981-07-18", 4),
			("Tran Thi Y", 56.78, "1999-05-12", 3),
			("Bui Xuan Z", NULL, NULL, 5);
```

```
		+----+--------------+--------+------------+--------+
		| id | name         | weight | birthday   | avatar |
		+----+--------------+--------+------------+--------+
		|  1 | Nguyen Van X |  62.54 | 1981-07-18 |      4 |
		|  2 | Tran Thi Y   |  56.78 | 1999-05-12 |      3 |
		|  3 | Bui Xuan Z   |   NULL | NULL       |      5 |
		+----+--------------+--------+------------+--------+
```
First, `#include <mysql+++.h>` whereever you want to use - Note that this is the ONLY step to do with your projects, nothing else!


### Establishing a connection by constructor, or by `connection::open()` function
```cpp
	connection my{ "localhost", "tester", "tester", "test_test_test" };
	if (!my) {
		cout << "Connection failed" << endl;
		return;
	}
```


### Simple query and simple way to get back a single value
```cpp
	auto row_count = my.query("select count(*) from person")
						.get_value<int>();
	cout << "Count: " << row_count << endl;
```


### Example of simple query without getting any value back using `exec()`
Note that `query()` also works here but is less performant, as the returned `results` object when not stored is automatically destroyed.
```cpp
	if (my.exec("update person set avatar = floor(rand()*10)"))
		cout << "Successful" << endl;
	else cout << "Failed" << endl;
```


### Queries can be passed using convenient printf-style, and `optional` type can be used to know whether the returned value is available
```cpp
	auto avg_weight = my.query("select avg(weight) from person where avatar >= %d or weight <= %f", 2, 70.5)
						.get_value<optional<double>>();
	if (avg_weight)
		cout << "Mean weight: " << *avg_weight << endl;
	else cout << "No mean weight value" << endl;
```


### Example showing how to getting back multiple typed values in a row, `optional` type is also used to handle nullable values
```cpp
	int id;
	string name;
	optional<double> weight;

	auto res = my.query("select id, name, weight from person where id = %d", 3);
	if (res) {
		res.fetch(id, name, weight);

		cout << "ID: " << id << ", name: " << name;
		if (weight) cout << ", weight: " << *weight;
		cout << endl;
	}
```


### A multi-row data query using lambda function with row fields in parameter
Note that `optional` type can be used here as well, and `datetime` is a supporting type is used for handing date and time.
```cpp
	my.query("select id, name, weight, birthday from person")
		.each([](int id, string name, optional<double> weight, optional<datetime> birthday) {
			// ...

			// return true to continue, false to stop
			return true;
		});
```


### Another way to iterate through rows using a container-like object
Note that unsuccessful/no-result queries will return an empty container.
```cpp
	res = my.query("select id, name, weight from person");
	for (auto row : res.as_container<int, string, optional<double>>()) {
		tie(id, name, weight) = row;
		// ...
	}
```


### or in C++98 style
```cpp
	res = my.query("select id, name, weight from person");
	auto ctn = res.as_container<int, string, optional<double>>();
	for (auto i = ctn.begin(); i != ctn.end(); i++) {
		tie(id, name, weight) = *i;
		// ...
	}
```


### Of course, the-old-good-time-style loop is also possible
```cpp
	res = my.query("select id, name, weight from person");
	while (!res.eof()) {
		res.fetch(id, name, weight);
		// ...
		res.next();
	}
```


### Use `mquery` to handle multiple queries, or queries that returns more than one dataset in an execution
Make sure to enable `CLIENT_MULTI_STATEMENTS` first using `connect_options::client_flags` when connecting, or using `set_server_option(MYSQL_OPTION_MULTI_STATEMENTS_ON)` after connection.
```cpp
	my.set_server_option(MYSQL_OPTION_MULTI_STATEMENTS_ON);
	auto datasets = my.mquery("select count(*) from person; select id, name, weight from person");

	cout << datasets[0].get_value<int>(0) << endl;

	datasets[1].each([](int id, string name, optional<double> weight) {
		cout << "ID: " << id << ", name: " << name;
		if (weight) cout << ", weight: " << *weight;
		cout << endl;
		return true;
	});
```


### Prepared statements are also supported
(Thanks @Thalhammer for excellent implementation)

```cpp
	prepared_stmt stmt(my, "select id, name, weight from person where weight > ? or weight is null");

	double pweight = 60;
	stmt.bind_param(pweight);
	stmt.bind_result(id, name, weight);
	stmt.execute();
	while (stmt.fetch()) {
		// ...
	}
```
