#include<iostream>
#include<time.h>
#include<cassert>
#include<stack>
#include<vector>

#include"memorypool.h"
#include"memorypool.cpp"
using namespace std;

class testdata{
private:
	int a = 100;
public:
	testdata() {};
	testdata(int data) :a(data) {};
};
#define ELEMS 100000
#define REPS 50
int main()
{
	clock_t  start;

	MemoryPool<testdata>pool;
	//cout << pool.max_size() << endl;
	start = clock();
	for (int i = 0; i < REPS; i++)
	{
		for (int j = 0; j < ELEMS; j++)
		{
			testdata a(0);
			testdata *p = pool.newElement(a);
			pool.deleteElement(p);
		}
	}
	cout << "MemoryPool run time : " << (((double)clock() - start) / CLOCKS_PER_SEC) << endl;

	start = clock();
	for (int i = 0; i < REPS; ++i)
	{
		for (int j = 0; j< ELEMS; ++j)
		{
			testdata *x = new testdata(1);

			delete x;
		}
	}
	cout << "new()/delete() run time: "<< (((double)clock() - start) / CLOCKS_PER_SEC) <<endl;

	while (1);
	return 0;
}
