#include <bits/stdc++.h>
#include <iostream>

using namespace std;

vector<pair<int, vector<string>>> register_students(vector<string> classes)
{
	vector<pair<int, vector<string>>> students;

	for (int i = 0; i < classes.size(); i++) {
		pair<int, vector<string>> student;
		student.first = rand() % 100;
		for (int i = 0; i < 3; i++) {
			student.second.push_back(classes[rand() % classes.size()]);
		}
		students.push_back(student);
	}

	return students;
}

void show_students(vector<pair<int, vector<string>>> students)
{
	for (vector<pair<int, vector<string>>>::iterator it = students.begin(); it != students.end(); it++) {
		cout << "Student ID: " << it->first << ", classes: ";
		for (vector<string>::iterator class_it = it->second.begin(); class_it != it->second.end(); class_it++) {
			cout << *class_it << " ";
		}
		cout << endl;
	}
}

map<string, vector<int>> group_by_class(vector<pair<int, vector<string>>> students)
{
	map<string, vector<int>> m;

	for (vector<pair<int, vector<string>>>::iterator student_it = students.begin(); student_it != students.end(); ++student_it) {
		for (vector<string>::iterator class_it = student_it->second.begin(); class_it != student_it->second.end(); ++class_it) {
			map<string, vector<int>>::iterator it = m.find(*class_it);
			if (it == m.end()) {
				vector<int> student_id;
				student_id.push_back(student_it->first);
				m.insert(pair<string, vector<int>>(*class_it, student_id));
			} else {
				it->second.push_back(student_it->first);
			}
		}
	}
	return m;
}

void show_by_class(map<string, vector<int>> m)
{
	for (map<string, vector<int>>::iterator it = m.begin(); it != m.end(); ++it) {
		cout << "Class: " << it->first << ", ID: ";
		for (vector<int>::iterator v = it->second.begin(); v != it->second.end(); ++v) {
			cout << *v << ", ";
		}
		cout << endl;
	}
}

int main()
{
	vector<string> classes {"writing101", "calculus1a", "bio1a", "cs64a",
													"history1a", "physics4a", "pe1", "english1a",
													"socio1B", "chemistry1c"};
	vector<pair<int, vector<string>>> students = register_students(classes);
	show_students(students);
	map<string, vector<int>> m = group_by_class(students);
	show_by_class(m);
}