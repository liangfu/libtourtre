#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

extern "C" 
{
#include <tourtre.h>
}

#include "Data.h"
#include "Mesh.h"

#include <unistd.h>

using std::cin;
using std::cout;
using std::clog;
using std::cerr;
using std::endl;
using std::vector;


double value ( size_t v, void * d ) {
	Mesh * mesh = reinterpret_cast<Mesh*>(d);
	return mesh->data[v];
}


size_t neighbors ( size_t v, size_t * nbrs, void * d ) {
	Mesh * mesh = static_cast<Mesh*>(d);
	static std::vector<size_t> nbrsBuf;
	
	nbrsBuf.clear();
	mesh->getNeighbors(v,nbrsBuf);
	
	for (uint i = 0; i < nbrsBuf.size(); i++) {
		nbrs[i] = nbrsBuf[i]; 
	}
	return nbrsBuf.size();
}


void outputNode( std::ofstream & out, ctBranch * b, Data * data, vector<int> & pool, vector<int> & invis ) {
  pool.push_back(b->extremum);
  pool.push_back(b->saddle);
  invis.push_back((int)data->data[b->extremum]);
  invis.push_back((int)data->data[b->saddle]);

	for ( ctBranch * c = b->children.head; c != NULL; c = c->nextChild ){
		outputNode( out, c, data, pool, invis );
	}
} 

#define HEIGHT      32


void removeNonExtremum( vector<vector<int> > & pool, Data * data, vector<int> & invis, vector<int> & nodes ) {
  for (int i = 0; i < pool.size(); i++) {
    for (int j = 0; j < pool[i].size(); j+=pool[i].size()-1) {
      int idx = pool[i][j];
      invis.push_back((int)data->data[idx]);
      nodes.push_back(idx);
    }
  }
}


void printNode( std::ofstream & out, vector<vector<int> > & pool, Data * data ) {
  for (int i = 0; i < pool.size(); i++) {
    for (int j = 0; j < pool[i].size(); j++) {
      int idx = pool[i][j];
      out << "  node" << idx;
      out << "[";
      out << "pos=\"" << idx   % HEIGHT << "," << HEIGHT - (idx   / HEIGHT) << "!\"";
      out << ", ";
      out << "label=\"" << idx   % HEIGHT << "," << idx   / HEIGHT << "\\n" << (int)data->data[idx] << "\"";
      out << "]\n";
      out << "  invis" << (int)data->data[idx] << "[style=invis];\n";
      // invis.push_back((int)data->data[idx]);
      // nodes.push_back(idx);
    }
  }
}


void outputRank( std::ofstream & out, vector<int> & nodes, Data * data, int rank, vector<int> & vpool ) {
  for (int i = 0; i < nodes.size(); i++) {
    // for (int j = 0; j < pool[i].size(); j++) {
      int index = nodes[i];
      if (data->data[index] == rank) {
        vpool.push_back(index);
      }
    // }
  }
} 


void outputTree( std::ofstream & out, ctBranch * b, Data * data, vector<vector<int> > & pool ) {
  vector<int> nodes;
	nodes.push_back(b->saddle);
	for ( ctBranch * c = b->children.head; c != NULL; c = c->nextChild ){
    nodes.push_back(c->saddle);
	}
	nodes.push_back(b->extremum);
  pool.push_back(nodes);
  
	for ( ctBranch * c = b->children.head; c != NULL; c = c->nextChild ){
		outputTree( out, c, data, pool );
	}
} 

int get_max_elem(vector<int> & pool, Data * data) {
  int a = data->data[*pool.begin()];
  int b = data->data[*pool.rbegin()];
  return std::max(a, b);
}


void sortTree(vector<vector<int> > & pool, Data * data) {
  // sort values in a branch
  for (int i = 0; i < pool.size(); i++) {
    for (int j = 0; j < pool[i].size(); j++) {
      for (int k = 0; k < j; k++) {
        if (data->data[pool[i][j]] > data->data[pool[i][k]]) {
          int tmp = pool[i][j];
          pool[i][j] = pool[i][k];
          pool[i][k] = tmp;
        }
      }
    }
  }
  // remove self reference
  for (int i = 0; i < pool.size(); i++) {
    std::vector<int>::iterator it = std::unique(pool[i].begin(), pool[i].end());
    pool[i].resize( std::distance(pool[i].begin(), it) );
  }
  // sort branches by peak value
  for (int i = 0; i < pool.size(); i++) {
    for (int j = 0; j < i; j++) {
      if (get_max_elem(pool[i], data) > get_max_elem(pool[j], data)) {
        vector<int> tmp = pool[i];
        pool[i] = pool[j];
        pool[j] = tmp;
      }
    }
  }
}


void printTree( std::ofstream & out, Data * data, vector<vector<int> > & pool, vector<int> & nodes ) {
  for (int i = 0; i < pool.size(); i++) {
    out << "  node" << pool[i][0];
    for ( int j = 1; j < pool[i].size(); j++ ){
      if (find(nodes.begin(), nodes.end(), pool[i][j]) != nodes.end()) {
        out << " -> node" << pool[i][j];
      }
    }
    out << "\n";
    
    out << "  // " << (int)data->data[pool[i][0]];
    for ( int j = 1; j < pool[i].size(); j++ ){
      if (find(nodes.begin(), nodes.end(), pool[i][j]) != nodes.end()) {
        out << " -> " << (int)data->data[pool[i][j]];
      }
    }
    out << "\n";
  }
}


//                  figure     75x20x1        uint16
// void process(uint16 * data, int shape[3], int dtype, vector<vector<int> > & pool) {
// }

int main( int argc, char ** argv ) {
	int errflg = 0;
	int c;
	
	//command line parameters
	char filename[1024] = "";
	char outfile[1024] = "";
	
	#if USE_ZLIB
	char switches[256] = "i:o:";
	#else
	char switches[256] = "i:o:";
	#endif

	while ( ( c = getopt( argc, argv, switches ) ) != EOF ) {
		switch ( c ) {
				case 'i': {
					strcpy(filename,optarg);
					break;
				}
				case 'o': {
					strcpy(outfile,optarg);
					break;
				}
				
				case '?':
					errflg++;
		}
	}
		
	if ( errflg || filename[0] == '\0') {
		clog << "usage: " << argv[0] << " <flags> " << endl << endl;
		
		clog << "flags" << endl;
		clog << "\t -i < filename >  :  filename" << endl;
		clog << "\t -o < filename >  :  filename" << endl;
		clog << endl;

		clog << "Filename must be of the form <name>.<i>x<j>x<k>.<type>" << endl;
		clog << "i,j,k are the dimensions. type is one of uint8, uint16, float or double." << endl << endl;
		clog << "eg, \" turtle.128x256x512.float \" is a file with 128 x 256 x 512 floats." << endl;
		clog << "i dimension varies the FASTEST (in C that looks like \"array[k][j][i]\")" << endl;
		clog << "Data is read directly into memory -- ENDIANESS IS YOUR RESPONSIBILITY." << endl;

		return(1);
	}


	char prefix[1024];

	//Load data
	Data data;
	bool compress;
	if (!data.load( filename, prefix, &compress )) {
		cerr << "Failed to load data" << std::endl;
		exit(1);
	}

	//Create mesh
	Mesh mesh(data);
	std::vector<size_t> totalOrder;
	mesh.createGraph( totalOrder ); //this just sorts the vertices according to data.less()
	
	//init libtourtre
	ctContext * ctx = ct_init(
		data.totalSize, //numVertices
		&(totalOrder.front()), //totalOrder. Take the address of the front of an stl vector, which is the same as a C array
		&value,
		&neighbors,
		&mesh //data for callbacks. The global functions less, value and neighbors are just wrappers which call mesh->getNeighbors, etc
	);
	
	//create contour tree
	ct_sweepAndMerge( ctx );
	ctBranch * root = ct_decompose( ctx );
	
	//create branch decomposition
	ctBranch ** map = ct_branchMap(ctx);
	ct_cleanup( ctx );
	
	//output tree
	std::ofstream out(outfile,std::ios::out);
	if (out) {
    out << "digraph g {\n";
    out << "  graph[ranksep=\"0.1\"]\n";
    out << "  rankdir=\"TB\"\n";

    vector<int> invis;
    vector<vector<int> > ipool;
    vector<vector<int> > pool;
    
		outputTree( out, root, &data, ipool);
    for (auto it = ipool.begin(); it < ipool.end(); it++){
      if ((*it).size() > 2) { pool.push_back(*it); }
    }
    
    sortTree( pool, &data );
    vector<int> nodes;
    removeNonExtremum(pool, &data, invis, nodes);

    printNode(out, pool, &data);

    // print invis
    std::sort(invis.begin(), invis.end());
    std::reverse(invis.begin(), invis.end());
    out << "  invis" << invis[0];
    for (int i = 1; i < invis.size(); i++) {
      if (invis[i-1] != invis[i]) { out << " -> invis" << invis[i]; }
    }
    out << " [style=invis];\n";

    printTree( out, &data, pool, nodes );

    for (int rank = 0; rank < 32767; rank++) {
      vector<int> vpool;
      outputRank( out, nodes, &data, rank, vpool);
      if (vpool.size() > 0) {
        out << "  subgraph g" << rank << "{\n";
        out << "    rank=\"same\"\n";
        for (int i = 0; i < vpool.size(); i++) {
          out << "    node" << vpool[i] << "\n";
        }
        out << "    invis" << rank << "\n";
        out << "  }\n";
      }
    }
    
    out << "}\n";
	} else {
		cerr << "couldn't open output file " << outfile << endl;
	}
	
}



	
