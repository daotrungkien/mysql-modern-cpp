# cpp11-mysql

## Description:
This is a lightweight wrapper for MySQL with simple and convenient usage in modern C++ (C++11, C++14, C++17).

## License:
Completely free. No restriction!

## Installation:
- C++11 required
- `#include <modern_cpp_mysql.h>` whereever you want to use - Note that this is the ONLY step to do, nothing else needed!
- No .cpp, no complied library, no binary needed

## Credits:
- `std::experimental::optional` is done by @github/akrzemi1: https://github.com/akrzemi1/Optional

## How to use (by examples):
Note that in the following example, a table defined as follows is used:

		create table(
			id unsigned int auto_increment primary key,
			name varchar(50),
			weight double,
			birthday date,
			avatar int);
		);

		+----+--------------+--------+------------+--------+
		| id | name         | weight | birthday   | avatar |
		+----+--------------+--------+------------+--------+
		|  1 | Nguyen Van X |  62.54 | 1981-07-18 |      4 |
		|  2 | Tran Thi Y   |  56.78 | 1999-05-12 |      3 |
		|  3 | Bui Xuan Z   |   NULL | NULL       |      5 |
		+----+--------------+--------+------------+--------+

Establishing a connection by constructor, or by connection::open() function:
```cpp
	connection my("db4free.net", "kien", "kienkien", "kien");
	if (!my) {
		cout << "Connection failed" << endl;
		return;
	}
```
Example of simple query, without getting any value back (the returned object when not assigned is automatically destroyed):
```cpp
	if (my.query("update person set avatar = floor(rand()*10)"))
		cout << "Successful" << endl;
	else cout << "Failed" << endl;
```
Simple query and simple way to get back a single value:
```cpp
	auto row_count = my.query("select count(*) from person")
						.get_value<int>();
	cout << "Count: " << row_count << endl;
```

Queries can be passed using convenient printf-style, and optional type can be used to know whether the returned value is available:
```cpp
	auto avg_weight = my.query("select avg(weight) from person where avatar >= %d or weight <= %f", 2, 70.5)
						.get_value<optional_type<double>>();
	if (avg_weight)
		cout << "Mean weight: " << *avg_weight << endl;
	else cout << "No max weight value" << endl;
```

Example showing how to getting back multiple typed values in a row, optional type is also used to handle nullable values:
```cpp
	auto res = my.query("select id, name, weight from person where id = %d", 3);

	int id;
	string name;
	optional_type<double> weight;
	res.fetch(id, name, weight);

	cout << "ID: " << id << ", name: " << name;
	if (weight) cout << ", weight: " << *weight;
	cout << endl;
```
A multi-row data query using lambda function with row fields in parameter. Note that optional type can be used here as well, and datetime is a supporting type is used for handing date and time:
```cpp
	my.query("select id, name, weight, birthday from person")
		.each([](int id, string name, optional_type<double> weight, optional_type<datetime> birthday) {
			cout << "ID: " << id << ", name: " << name;
			if (weight) cout << ", weight: " << *weight;
			if (birthday) cout << ", birthday: " << birthday->to_sql();
			cout << endl;

			// return true to continue, false to stop
			return true;
		});
```

Another way to iterate through rows using a container-like object:
```cpp
	res = my.query("select id, name, weight from person");
	auto ctn = res.as_container<int, string, optional_type<double>>();
	for (auto row : ctn) {
		tie(id, name, weight) = row;

		cout << "ID: " << id << ", name: " << name;
		if (weight) cout << ", weight: " << *weight;
		cout << endl;
	}
```
