#include <iostream>
#include <vector>
#include <fstream>
#include <string>
using namespace std;

long maxInversion(vector<int> prices)
{
	vector<int> v;
	long count = 0;
	
	for (int i=0; i< prices.size()-2; i++) {
		v.push_back(prices[i]);
		for (int j=i+1; j<prices.size(); j++) {
			if (prices[j] < v[v.size()-1]) {
				cout << "push: " << prices[j] << endl;
				v.push_back(prices[j]);
			} else {
				if (v.size() >= 3) {
					count += v.size()-2;
					for (vector<int>::iterator it = v.begin(); it!=v.end(); it++) {
						cout << "sub: " << *it << " ";
					}
					cout << endl;
				}
				while (v.size() > 1) {
					if (v[v.size()-1] <= prices[j]) {
						cout << "pop: " << v[v.size()-1] << endl;
						v.pop_back();
					} else {
						break;
					}
				}
				if (prices[j] < v[v.size()-1]) {
					v.push_back(prices[j]);
					cout << "push: " << prices[j] << ", size: " << v.size() << endl;
				}
			}
		}
		if (v.size() >= 3) {
			count += v.size()-2;
			for (vector<int>::iterator it = v.begin(); it!=v.end(); it++) {
				cout << "sub: " << *it << " ";
			}
			cout << endl;
		}

		v.clear();
	}
	return count;
}

long maxInversion1(vector<int> prices)
{
	long count = 0;
	for (int i=1; i < prices.size(); i++) {
		int left = 0, right = 0;
		for (int j=0; j < i; j++) {
			if (prices[j] > prices[i])
				left++;
		}
		for (int j=i+1; j < prices.size(); j++) {
			if (prices[j] < prices[i])
				right++;
		}
		count += right * left;
	}
	return count;
}

int main()
{
	vector<int> prices;
	int n;
	cin >> n;
	for (int i=0; i<n; i++) {
		int p;
		cin >> p;
		prices.push_back(p);
	}

	ofstream output;
	output.open("test.txt");
	output << "Testing C++ IO write";
	output.close();

	ifstream input("test.txt");
	string line;
	getline(input, line);
	cout << line;
	input.close();
	return 0;
}

