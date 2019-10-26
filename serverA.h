#ifndef SERVERA_H
#define SERVERA_H

#include <iostream>
#include <vector>
#include <cassert>
#include <string.h>
#include <map>
#include <stack>

using namespace std;

template<typename Weight>
class Edge{
private:
    int a,b;
    Weight weight;

public:
    Edge(int a, int b, Weight weight) {
        this->a = a;
        this->b = b;
        this->weight = weight;
    }

    Edge(){}

    ~Edge(){}

    // returns the first node
    int v() { return a; }
    // returns the seoncd node
    int w() { return b; }
    // returns the weight
    Weight wt() { return weight; }

    // returns the other node (given a specific node)
    int other(int x) {
        assert( x == a || x == b );
        return x == a ? b : a;
    }

    // Print the edge information
    friend ostream& operator<<(ostream &os, const Edge &e) {
        os<<e.a<<"-"<<e.b<<": "<<e.weight;
        return os;
    }

    // Compares the two edges
    bool operator<(Edge<Weight>& e) {
        return weight < e.wt();
    }
    bool operator<=(Edge<Weight>& e) {
        return weight <= e.wt();
    }
    bool operator>(Edge<Weight>& e) {
        return weight > e.wt();
    }
    bool operator>=(Edge<Weight>& e) {
        return weight >= e.wt();
    }
    bool operator==(Edge<Weight>& e) {
        return weight == e.wt();
    }
};

template<typename Weight>
class Graph {
private:
    int n, m, an; // an -> number of vertex, m -> number of edges
    double prop_speed, trans_speed;    // two speed information
    vector<vector<Edge<Weight> *> > g;  // The specific graph

public:
    Graph() {
        this->n = 0;
        this->an = 0;
        this->m = 0;
        this->prop_speed = 0;
        this->trans_speed = 0;
    }

    Graph(const Graph& otherG) {
        n = otherG.n;
        an = otherG.an;
        m = otherG.m;
        prop_speed = otherG.prop_speed;
        trans_speed = otherG.trans_speed;
        g = vector<vector<Edge<Weight> *> >(n, vector<Edge<Weight> *>());
    }

    ~Graph() { 
        for ( int i = 0 ; i < n ; i ++ ) 
            for ( int j = 0 ; j < g[i].size() ; j ++ ) 
                delete g[i][j];
    }

    void setV(int n) { 
        this->n = n; 
        g = vector<vector<Edge<Weight> *> >(n, vector<Edge<Weight> *>());
    }

    void setProp(double prop_speed) {
        this->prop_speed = prop_speed;
    }

    void setTrans(double trans_speed) {
        this->trans_speed = trans_speed;
    }

    int V() { return n; }
    int AN() { return an; }
    int E() { return m; }
    double prop() { return prop_speed; }
    double trans() { return trans_speed; }
    
    void addV(int v) {
        if (g[v].size() == 0) an++;
    }

    void addEdge(int v, int w, Weight weight) {
        assert(v >= 0 && v < n);
        assert(w >= 0 && w < n);

        g[v].push_back(new Edge<Weight>(v, w, weight));
        if (v != w)
            g[w].push_back(new Edge<Weight>(w, v, weight));
        m++;
    }

    bool hasEdge(int v , int w ) {
        assert( v >= 0 && v < n );
        assert( w >= 0 && w < n );
        for (int i = 0 ; i < g[v].size() ; i ++) {
            if (g[v][i]->other(v) == w) return true;
        }
        return false;
    }

    void show() {
        cout << "The Propagation Speed is:\t" << prop_speed << endl;
        cout << "The Transmission Speed is:\t" << trans_speed << endl;
        for (int i = 0 ; i < n ; i ++) {
            cout <<"vertex " << i << ":\t";
            for (int j = 0 ; j < g[i].size() ; j ++)
                cout << "( to:" << g[i][j]->w() << ",wt:" << g[i][j]->wt() << ")\t";
            cout << endl;
        }
        cout << "--------------" << endl << endl;
    }

    class adjIterator {
    private:
        Graph &G;
        int v, index;

    public:
        adjIterator(Graph &graph, int v): G(graph) {
            this->v = v;
            this->index = 0;
        }

        ~adjIterator() {}

        Edge<Weight>* begin() {
            index = 0;
            if (G.g[v].size())
                return G.g[v][index];
            return NULL;
        }

        Edge<Weight>* next() {
            index += 1;
            if (index < G.g[v].size())
                return G.g[v][index];
            return NULL;
        }

        bool end() {
            return index >= G.g[v].size();
        }
    };

};

template<typename Item>
class IndexMinHeap{
private:
    Item *data;
    int *indexes;
    int *reverse;

    int count;
    int capacity;

    void shiftUp(int k) {
        while (k > 1 && data[indexes[k/2]] > data[indexes[k]]) {
            swap(indexes[k/2], indexes[k]);
            reverse[indexes[k/2]] = k/2;
            reverse[indexes[k]] = k;
            k /= 2;
        }
    }

    void shiftDown(int k) {
        while (2*k <= count) {
            int j = 2 * k;
            if (j + 1 <= count && data[indexes[j]] > data[indexes[j+1]]) j += 1;
            if (data[indexes[k]] <= data[indexes[j]]) break;

            swap(indexes[k], indexes[j]);
            reverse[indexes[k]] = k;
            reverse[indexes[j]] = j;
            k = j;
        }
    }

public:

    IndexMinHeap(int capacity) {
        data = new Item[capacity+1];
        indexes = new int[capacity+1];
        reverse = new int[capacity+1];
        for (int i = 0 ; i <= capacity ; i ++)
            reverse[i] = 0;
        count = 0;
        this->capacity = capacity;
    }

    ~IndexMinHeap() {
        delete[] data;
        delete[] indexes;
        delete[] reverse;
    }

    int size() { return count; }

    bool isEmpty() { return count == 0; }

    void insert(int index, Item item) {
        assert(count + 1 <= capacity);
        assert(index + 1 >= 1 && index + 1 <= capacity);

        index += 1;
        data[index] = item;
        indexes[count+1] = index;
        reverse[index] = count+1;
        count++;
        shiftUp(count);
    }

    Item extractMin() {
        assert(count > 0);

        Item ret = data[indexes[1]];
        swap(indexes[1], indexes[count]);
        reverse[indexes[count]] = 0;
        reverse[indexes[1]] = 1;
        count--;
        shiftDown(1);
        return ret;
    }

    int extractMinIndex() {
        assert(count > 0);

        int ret = indexes[1] - 1;
        swap(indexes[1], indexes[count]);
        reverse[indexes[count]] = 0;
        reverse[indexes[1]] = 1;
        count--;
        shiftDown(1);
        return ret;
    }

    Item getMin() {
        assert(count > 0);
        return data[indexes[1]];
    }

    int getMinIndex() {
        assert(count > 0);
        return indexes[1]-1;
    }

    bool contain(int index) { return reverse[index+1] != 0; }

    Item getItem(int index) {
        assert(contain(index));
        return data[index+1];
    }

    void change(int index, Item newItem) {
        assert(contain(index));
        index += 1;
        data[index] = newItem;

        shiftUp(reverse[index]);
        shiftDown(reverse[index]);
    }
};

template<typename Graph, typename Weight>
class Dijkstra{
private:
    Graph &G;                  
    int s;                     
    Weight *distTo;            
    bool *marked;
    vector<Edge<Weight>* > from;

public:
    Dijkstra(Graph &graph, int s): G(graph) {
        assert(s >= 0 && s < G.V());

        this->s = s;
        distTo = new Weight[G.V()];
        marked = new bool[G.V()];
        for (int i = 0 ; i < G.V() ; i++) {
            distTo[i] = Weight();
            marked[i] = false;
            from.push_back(NULL);
        }

        IndexMinHeap<Weight> ipq(G.V());

        distTo[s] = Weight();
        from[s] = new Edge<Weight>(s, s, Weight());
        ipq.insert(s, distTo[s]);
        marked[s] = true;
        while (!ipq.isEmpty()) {
            int v = ipq.extractMinIndex();
            marked[v] = true;
            typename Graph::adjIterator adj(G, v);
            for (Edge<Weight>* e = adj.begin() ; !adj.end() ; e = adj.next()) {
                int w = e->other(v);
                if (!marked[w]) {
                    if (from[w] == NULL || distTo[v] + e->wt() < distTo[w]) {
                        distTo[w] = distTo[v] + e->wt();
                        from[w] = e;
                        if (ipq.contain(w))
                            ipq.change(w, distTo[w]);
                        else
                            ipq.insert(w, distTo[w]);
                    }
                }
            }
        }
    }

    ~Dijkstra() {
        delete[] distTo;
        delete[] marked;
        delete from[0];
    }

    Weight shortestPathTo(int w) {
        assert(w >= 0 && w < G.V());
        assert(hasPathTo(w));
        return distTo[w];
    }

    bool hasPathTo(int w) {
        assert(w >= 0 && w < G.V());
        return marked[w];
    }

    void shortestPath(int w, vector<Edge<Weight> > &vec) {
        assert(w >= 0 && w < G.V());
        assert(hasPathTo(w));

        stack<Edge<Weight>* > s;
        Edge<Weight> *e = from[w];
        while (e->v() != this->s) {
            s.push(e);
            e = from[e->v()];
        }
        s.push(e);
        while (!s.empty()) {
            e = s.top();
            vec.push_back(*e);
            s.pop();
        }
    }

    void showPath(int w) {
        assert(w >= 0 && w < G.V());
        assert(hasPathTo(w));

        vector<Edge<Weight> > vec;
        shortestPath(w, vec);
        for (int i = 0 ; i < vec.size() ; i++) {
            cout<<vec[i].v()<<" -> ";
            if (i == vec.size() - 1) cout << vec[i].w() << endl;
        }
    }
};

#endif
