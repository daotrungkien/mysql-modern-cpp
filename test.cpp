#include <iostream>

// uncomment the following line if needed:
#define NO_STD_OPTIONAL
#include "mysql+++/mysql+++.h"


using namespace std;
using namespace daotk::mysql;

int main()
{
	// connection
	connection my{ "localhost", "tester", "tester", "test_test_test" };
	if (!my) {
		cout << "Connection failed" << endl;
		return -1;
	}

	/* This testing program uses a table defined as follows:

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

		+----+--------------+--------+------------+--------+
		| id | name         | weight | birthday   | avatar |
		+----+--------------+--------+------------+--------+
		|  1 | Nguyen Van X |  62.54 | 1981-07-18 |      4 |
		|  2 | Tran Thi Y   |  56.78 | 1999-05-12 |      3 |
		|  3 | Bui Xuan Z   |   NULL | NULL       |      5 |
		+----+--------------+--------+------------+--------+
	*/



	int sample_count = 0;

	try {
		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// Simple query and simple way to get back a single value:
		auto row_count = my.query("select count(*) from person")
							.get_value<int>();
		cout << "Count: " << row_count << endl;




		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// Simple query without getting any value back (note that query() also
		// works here but is less performant, as the returned `result' object
		// when not stored is automatically destroyed):
		my.exec("update person set avatar = floor(rand()*10)");
		cout << "Successful" << endl;




		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// 1) queries can be passed using convenient printf-style
		// 2) `optional` type can be used to know whether the returned value is available
		auto avg_weight = my.query("select avg(weight) from person where avatar >= %d or weight <= %f", 2, 70.5)
							.get_value<optional<double>>();
		if (avg_weight)
			cout << "Mean weight: " << *avg_weight << endl;
		else cout << "No max weight value" << endl;


	

		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// Example showing how to getting back multiple typed values in a row:
		int id;
		string name;
		optional<double> weight;

		auto res = my.query("select id, name, weight from person where id = %d", 3);
		if (!res.is_empty()) {
			res.fetch(id, name, weight);

			cout << "ID: " << id << ", name: " << name;
			if (weight) cout << ", weight: " << *weight;
			cout << endl;
		}
		else cout << "Query failed" << endl;




		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// A multi-row data query using lambda function with row fields in parameter:
		// (`optional` type can also be used, and `datetime` is a supoorting type is used for handing date and time)
		my.query("select id, name, weight, birthday from person")
			.each([](int id, string name, optional<double> weight, optional<datetime> birthday) {
				cout << "ID: " << id << ", name: " << name;
				if (weight) cout << ", weight: " << *weight;
				if (birthday) cout << ", birthday: " << birthday->to_sql();
				cout << endl;

				// return true to continue, false to stop
				return true;
			});




		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// Another way to iterate through rows using a container-like object:
		// (Note that unsuccessful/no-result queries will return an empty container)
		res = my.query("select id, name, weight from person");
		for (auto& row : res.as_container<int, string, optional<double>>()) {
			weight.reset();
			tie(id, name, weight) = row;

			cout << "ID: " << id << ", name: " << name;
			if (weight) cout << ", weight: " << *weight;
			cout << endl;
		}




		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// Of course, the-old-good-time-style loop is also possible:
		res = my.query("select id, name, weight from person");
		while (!res.eof()) {
			weight.reset();
			res.fetch(id, name, weight);

			cout << "ID: " << id << ", name: " << name;
			if (weight) cout << ", weight: " << *weight;
			cout << endl;

			res.next();
		}




		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// Use `mquery` to handle multiple queries, or queries that returns more than one dataset in an execution;
		// make sure to enable `CLIENT_MULTI_STATEMENTS' first using `connect_options::client_flags` when connecting, or
		// using `set_server_option(MYSQL_OPTION_MULTI_STATEMENTS_ON)` after connection
		my.set_server_option(MYSQL_OPTION_MULTI_STATEMENTS_ON);
		auto datasets = my.mquery("select count(*) from person; select id, name, weight from person");
		
		cout << datasets[0].get_value<int>(0) << endl;
		
		datasets[1].each([](int id, string name, optional<double> weight) {
			cout << "ID: " << id << ", name: " << name;
			if (weight) cout << ", weight: " << *weight;
			cout << endl;
			return true;
		});



	
		cout << "** QUERY EXAMPLE " << ++sample_count << endl;

		// We also have support for prepared statements and binding:
		prepared_stmt stmt(my, "select id, name, weight from person where weight > ? or weight is null");

		double pweight = 60;
		stmt.bind_param(pweight);
		stmt.bind_result(id, name, weight);
		stmt.execute();
		while (stmt.fetch()) {
			cout << "ID: " << id << ", name: " << name;
			if (weight) cout << ", weight: " << *weight;
			cout << endl;
		}


	} catch (const mysql_exception& exp) {
		cout << "Query #" << sample_count << " failed with error: " << exp.error_number() << " - " << exp.what() << endl;
	}
	catch (const mysqlpp_exception& exp) {
		cout << "Query #" << sample_count << " failed with error: " << exp.what() << endl;
	}


	return 0;
}

