#include <iostream>
#include "mysql++11/mysql++11.h"


using namespace std;
using namespace daotk::mysql;

int main()
{
	// connection
	connection my("db4free.net", "kien", "kienkien", "kien");
	if (!my) {
		cout << "Connection failed" << endl;
		return -1;
	}

	/* This testing program uses a table defined as follows:

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
	*/



	int sample_count = 1;
	cout << "** QUERY EXAMPLE " << sample_count++ << endl;

	// example of simple query, without getting any value back
	// (the returned object when not assigned is automatically destroyed)

	if (my.query("update person set avatar = floor(rand()*10)"))
		cout << "Successful" << endl;
	else cout << "Failed" << endl;




	cout << "** QUERY EXAMPLE " << sample_count++ << endl;

	// simple query and simple way to get back a single value
	auto row_count = my.query("select count(*) from person")
						.get_value<int>();
	cout << "Count: " << row_count << endl;




	cout << "** QUERY EXAMPLE " << sample_count++ << endl;

	// 1) queries can be passed using convenient printf-style
	// 2) optional type can be used to know whether the returned value is available
	auto avg_weight = my.query("select avg(weight) from person where avatar >= %d or weight <= %f", 2, 70.5)
						.get_value<optional_type<double>>();
	if (avg_weight)
		cout << "Mean weight: " << *avg_weight << endl;
	else cout << "No max weight value" << endl;


	

	cout << "** QUERY EXAMPLE " << sample_count++ << endl;

	// example showing how to getting back multiple typed values in a row
	int id;
	string name;
	optional_type<double> weight;

	auto res = my.query("select id, name, weight from person where id = %d", 3);
	if (!res.is_empty()) {
		res.fetch(id, name, weight);

		cout << "ID: " << id << ", name: " << name;
		if (weight) cout << ", weight: " << *weight;
		cout << endl;
	} else cout << "Query failed" << endl;




	cout << "** QUERY EXAMPLE " << sample_count++ << endl;

	// a multi-row data query using lambda function with row fields in parameter,
	// note that optional type can also be used,
	// datetime is a supoorting type is used for handing date and time
	my.query("select id, name, weight, birthday from person")
		.each([](int id, string name, optional_type<double> weight, optional_type<datetime> birthday) {
			cout << "ID: " << id << ", name: " << name;
			if (weight) cout << ", weight: " << *weight;
			if (birthday) cout << ", birthday: " << birthday->to_sql();
			cout << endl;

			// return true to continue, false to stop
			return true;
		});




	cout << "** QUERY EXAMPLE " << sample_count++ << endl;

	// another way to iterate through rows using a container-like object
	// (unsuccessful/no-result queries will return an empty container)
	res = my.query("select id, name, weight from person");
	for (auto& row : res.as_container<int, string, optional_type<double>>()) {
		weight.reset();
		tie(id, name, weight) = row;

		cout << "ID: " << id << ", name: " << name;
		if (weight) cout << ", weight: " << *weight;
		cout << endl;
	}




	cout << "** QUERY EXAMPLE " << sample_count++ << endl;

	// of course, the-old-good-time-style loop is also possible:
	res = my.query("select id, name, weight from person");
	while (!res.eof()) {
		weight.reset();
		res.fetch(id, name, weight);

		cout << "ID: " << id << ", name: " << name;
		if (weight) cout << ", weight: " << *weight;
		cout << endl;

		res.next();
	}

	return 0;
}

