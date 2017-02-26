#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <time.h>

#include "md5.h"
#include "mht.h"

using namespace std;

static int hash_node(merkle_tree *mt, size_t i);
static void print_tree(merkle_tree *mt);

//build a merkle tree
int build_tree(merkle_tree *mt, unsigned char **data) {

	if (mt->data_blocks > (1 << (mt->tree_height - 1)))
		return -1;
	int i, leaf_start;
	leaf_start = (1 << (mt->tree_height - 1));
	mt->n = leaf_start + mt->data_blocks - 1;
	mt->nodes = (merkle_tree_node *)malloc(sizeof(merkle_tree_node)* (mt->n + 1));
	//assign them value
	for (i = leaf_start; i <= mt->n; i++) {
		mt->nodes[i].data = data[i - leaf_start];
		mt->nodes[i].hash = NULL;
		if (hash_node(mt, i) == -1)
			return -1;
	}
	for (i = leaf_start - 1; i > 0; i--) {
		mt->nodes[i].hash = NULL;
		if (hash_node(mt, i) == -1)
			return -1;
	}
	return 0;
}

//compare two merkle trees from node i
//return different data block number
//if no differnece return 0
int tree_cmp(merkle_tree *a, merkle_tree *b, size_t i) {

	int cmp;
	if (i > (1 << a->tree_height) - 1)
		return -1;
	if (memcmp(a->nodes[i].hash, b->nodes[i].hash, a->hash_size) != 0) {
		//if leaf return node num
		if (i << 1 > (1 << a->tree_height) - 1)
			return i - (1 << (a->tree_height - 1)) + 1;
		//else recursion
		else {
			cmp = tree_cmp(a, b, i << 1);
			if (cmp == 0)
				return tree_cmp(a, b, (i << 1) + 1);
			else
				return cmp;
		}
	}
	else
		return 0;
}

// set tree data with specific block number
int set_tree_data(merkle_tree *mt, size_t block_num, unsigned char *data) {

	if (block_num > mt->data_blocks)
		return -1;
	size_t i = (1 << (mt->tree_height - 1)) + block_num - 1;
	if (mt->nodes[i].data)
		mt->nodes[i].data = NULL;
	mt->nodes[i].data = data;
	if (hash_node(mt, i) == -1)
		return -1;
	for (i >>= 1; i>0; i >>= 1)
	if (hash_node(mt, i) == -1)
		return -1;
	return 0;
}

//free the Merkle Tree Object
void freeMerkleTree(merkle_tree *mt) {

	int i;
	if (!mt)
		return;
	if (mt->nodes) {
		for (i = 1; i <= mt->n; i++)
		if (mt->nodes[i].hash)
			free(mt->nodes[i].hash);
		free(mt->nodes);
	}
	return;
}


//update a tree node hash
//leaf & inside nodes 
static int hash_node(merkle_tree *mt, size_t i) {

	if (i > (1 << mt->tree_height) - 1)
		return -1;
	//inside nodes
	if (i < (1 << mt->tree_height - 1)){
		//two children
		if (2 * i + 1 <= mt->n && mt->nodes[2 * i].hash && mt->nodes[2 * i + 1].hash) {
			unsigned char *buffer = (unsigned char *)malloc(sizeof(char *)* (2 * mt->hash_size + 1));
			//get their hash
			memcpy(buffer, mt->nodes[2 * i].hash, mt->hash_size);
			memcpy(buffer + mt->hash_size, mt->nodes[2 * i + 1].hash, mt->hash_size);
			if (!mt->nodes[i].hash)
				mt->nodes[i].hash = (unsigned char *)malloc(sizeof(char *)* mt->hash_size);
			mt->hash_function(buffer, 2 * mt->hash_size, mt->nodes[i].hash);
			free(buffer);
		}
		//one
		else if (2 * i <= mt->n && mt->nodes[2 * i].hash) {
			if (!mt->nodes[i].hash)
				mt->nodes[i].hash = (unsigned char *)malloc(sizeof(char *)* mt->hash_size);
			memcpy(mt->nodes[i].hash, mt->nodes[2 * i].hash, mt->hash_size);
		}
	}
	//leaf
	else {
		if (mt->nodes[i].data) {
			if (!mt->nodes[i].hash)
				mt->nodes[i].hash = (unsigned char *)malloc(sizeof(char *)* mt->hash_size);
			mt->hash_function(mt->nodes[i].data, mt->data_block_size, mt->nodes[i].hash);
		}
		else
			return -1;
	}
	return 0;
}

//for test use
//print a merkle tree nodes hash
static void print_tree(merkle_tree *mt) {

	int i;
	printf("--------------------------------\n");
	for (i = 1; i <= mt->n; i++) {
		printf("%d: ", i);
		MD5Print(mt->nodes[i].hash);
	}
	printf("--------------------------------\n");
	return;
}


//merkle tree test
#define BLOCK_SIZE 4096

merkle_tree loadfile(char* filename){
	
	//load file
	ifstream in(filename, ios::in | ios::binary);

	//get length
	long s, e, len;
	s = in.tellg();
	in.seekg(0, ios::end);
	e = in.tellg();
	len = e - s;
	in.seekg(0, ios::beg);
	cout << "file size: "<< len << endl;
	//get height & data blocks
	int height = 1, data_blocks = 1;
	while (data_blocks*BLOCK_SIZE < len) {
		data_blocks *= 2;
		height++;
	}
	data_blocks = len / BLOCK_SIZE + 1;

	//read file & build tree
	merkle_tree mt = { 0, height, MD5_DIGEST_LENGTH, BLOCK_SIZE, data_blocks, MD5, NULL };
	unsigned char** data;
	data = new unsigned char*[data_blocks];
	for (int i = 0; i < data_blocks; i++){
		data[i] = new unsigned char[BLOCK_SIZE];
	}

	if (in.bad()){
		cout << "false" << endl;
	}
	else {
		int j = 0;
		while (!in.eof()){
			in.read((char*)data[j++], sizeof(char)*BLOCK_SIZE);
			//cout << buf;
		}
	}

	//close file
	in.close();

	build_tree(&mt, data);

	for (int i = 0; i <data_blocks; i++)
	{
		delete[] data[i];
	}
	delete[] data;

	return mt;
}

int main()
{
	char filename[128];

	clock_t s, e;
	cout << "input the path of file: ";
	cin >> filename;
	s = clock();
	merkle_tree mt_1 = loadfile(filename);
	e = clock();
	cout << "time cost: ";
	cout << (double)(e - s) / CLOCKS_PER_SEC << "s" << endl;


	cout << "copying to tree2..." << endl;
	merkle_tree mt_2 = loadfile(filename);
	cout << "using buffer replace block 2" << endl;
	unsigned char buf[BLOCK_SIZE];
	for (int i = 0; i < BLOCK_SIZE; i++){
		buf[i] = 'A';
	}
	set_tree_data(&mt_2, 2, buf);


	//print_tree(&mt_1);
	//print_tree(&mt_2);

	printf("total blocks: %d\n", mt_1.data_blocks);
	printf("the differnt block is (0 for no different) : %d\n", tree_cmp(&mt_1, &mt_2, 1));

	freeMerkleTree(&mt_1);
	freeMerkleTree(&mt_2);

	system("PAUSE");

	return 0;
}
