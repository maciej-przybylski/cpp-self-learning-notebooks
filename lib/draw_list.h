#ifndef DRAW_LIST_H
#define DRAW_LIST_H

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <set>
#include <queue>
#include "xcpp/xdisplay.hpp"

#include "nlohmann/json.hpp"

namespace sll{

template <typename T, typename U>
std::vector<std::pair<T, U>> zip(const std::vector<T>& a, const std::vector<U>& b) {
    std::vector<std::pair<T, U>> result;
    size_t n = std::min(a.size(), b.size());
    result.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        result.emplace_back(a[i], b[i]);
    }
    return result;
}


// helper to trim leading/trailing whitespace
inline std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        ++start;
    }

    auto end = s.end();
    do {
        --end;
    } while (end != start && std::isspace(*end));

    return std::string(start, end + 1);
}

// split by commas, trim, and return vector
inline std::vector<std::string> split_csv(std::string input) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, ',')) {
        auto cleaned = trim(token);
        if (!cleaned.empty()) { // optional: skip empty tokens
            result.push_back(cleaned);
        }
    }

    return result;
}

std::vector<std::string> push_front(std::string s, std::vector<std::string> ss)
{
    ss.insert(ss.begin(), s);
    return ss;
}

}//namespace sll

#define P_NAMES(P1, ...) sll::push_front(#P1,  sll::split_csv(#__VA_ARGS__))
#define POINTERS(P1, ...) sll::zip(std::vector<std::remove_reference_t<decltype(P1)>>({P1,  __VA_ARGS__}), P_NAMES(P1,  __VA_ARGS__))
#define DRAW_LIST(P1, ...) drawList<std::remove_reference_t<decltype(*P1)>>(POINTERS(P1, __VA_ARGS__))
#define DRAW_LIST_UPDATED(delay, ID, P1, ...) drawList<std::remove_reference_t<decltype(*P1)>>(POINTERS(P1, __VA_ARGS__), ID, delay)

namespace nl = nlohmann;

namespace ht
{
    struct html
    {   
        inline html(const std::string& content)
        {
            m_content = content;
        }
        std::string m_content;
    };

    nl::json mime_bundle_repr(const html& a)
    {
        auto bundle = nl::json::object();
        bundle["text/html"] = a.m_content;
        return bundle;
    }
}

template <typename T>
struct Node {
    T dane;
    Node* nast;
    Node(T d, Node* n=nullptr) : dane(d), nast(n) {}
};


template <typename NodeT>
std::string addressToText(NodeT* p){
    std::string addr;
    if (p) {
        std::ostringstream addr_out;
        addr_out << p;
        int addr_len = addr_out.str().size();
        addr = addr_out.str().substr(0,3) + ".." + addr_out.str().substr(addr_len-4,addr_len);
        
    } else {
        addr = "NULL";
    }
    return addr;
}

template <typename NodeT>
void drawArrow(std::ostream &out, int x_start, int x_end, int y, NodeT *addr, std::string label){
    int spacing = x_end - x_start;
    
    out << "  <text x='" << (x_start + spacing/2) << "' y='" 
            << (y + 15)
            << "' text-anchor='middle' font-size='10'>" << addressToText(addr) << "</text>\n";

        out << "  <line x1='" << x_start << "' y1='" << y
            << "' x2='" << x_end << "' y2='" << y
            << "' stroke='black' marker-end='url(#sll_arrow)'/>\n";

        out << "  <text x='" << (x_start + spacing/2)
            << "' y='" << (y - 10)
            << "' text-anchor='middle'>"+label+"</text>\n";
}


template <typename NodeT>
void drawPath(std::ostream &out, std::vector<std::pair<int, int>> path, NodeT *addr, std::string label){
    int spacing = path.back().first - path.front().first;
    int x_start = path.front().first;
    int y = path.front().second;
    
    out << "  <text x='" << (x_start + spacing/2) << "' y='" 
            << (y + 15)
            << "' text-anchor='middle' font-size='10'>" << addressToText(addr) << "</text>\n";

    for(int i=0; i+1<path.size(); ++i)
    {
        auto p1 = path[i];
        auto p2 = path[i+1];
        out << "  <line x1='" << p1.first << "' y1='" << p1.second
            << "' x2='" << p2.first << "' y2='" << p2.second;
        if(i+2<path.size())
            out << "' stroke='black' />\n";
        else
            out << "' stroke='black' marker-end='url(#sll_arrow)'/>\n";
    }

    out << "  <text x='" << (x_start + spacing/2)
        << "' y='" << (y - 10)
        << "' text-anchor='middle'>"+label+"</text>\n";
}

template <typename NodeT>
void drawNode(std::ostream &out, int x, int y, int dataWidth, int nodeHeight, NodeT *p)
{
    if(!p) return;
    // Full node rectangle (data)
    out << "  <rect x='" << x << "' y='" << y
        << "' width='" << (dataWidth) << "' height='" << nodeHeight
        << "' fill='white' stroke='black'/>\n";

    // Data value (centered in left box)
    out << "  <text x='" << (x + dataWidth/2) << "' y='" << (y + nodeHeight/2 + 5)
        << "' text-anchor='middle'>" << p->dane << "</text>\n";
}

template <typename NodeT>
struct PointerProperties{
    NodeT *p;
    std::string label;
    int i,j;    
};


template <typename NodeT>
struct TreeNode{

    struct Edge{
        NodeT *from, *to;
        std::string label;
        int j{0};
    };
    std::vector<Edge> predecessors;
    NodeT *p{nullptr};
    int i{0},j{0};    

    void addPredecessorUnique(Edge const& e)
    {
        bool not_in = true;
        for(auto pred : predecessors)
            if(pred.from == e.from)
            {
                not_in = false;
                break;
            }
        if(not_in)
            predecessors.push_back(e);
    }
    
};


template <typename NodeT>
std::string generateLinkedListSVG(std::vector<std::pair<NodeT*, std::string>> pointers={}) {
    std::map<NodeT*, TreeNode<NodeT>> tree_nodes;
    std::set<NodeT*> last_elements;
    std::vector<std::pair<NodeT*, std::string>> null_pointers;
    NodeT* deepest_node = nullptr;
    int deepest_i = -1;

    // Initialize tree
    for(auto pp : pointers)
    {
        NodeT* p = pp.first;

        if(!p)
        {
            null_pointers.push_back(pp);
            continue;
        }
        int i=0;

        if(!deepest_node)
        {
            deepest_node = p;
            deepest_i = i;
        }
        

        tree_nodes[p].p = p;
        tree_nodes[p].predecessors.push_back({nullptr,p, pp.second,0});

        while(p)
        {
            ++i;
            auto pn = p->nast;
            
            if(pn)
            {
                tree_nodes[pn].p = pn;
                if(i>tree_nodes[pn].i)
                    tree_nodes[pn].i = i;
                tree_nodes[pn].addPredecessorUnique({p,pn, "nast",0});

                if(deepest_i < i)
                {
                    deepest_node = pn;
                    deepest_i = i;
                }
            }
            else
            {
                last_elements.insert(p);
            }
            p = pn;
        }
    }
        
    int nc = deepest_i>1?deepest_i:1;
    int nr = 0;

    if(!last_elements.empty())
    {
        std::vector<std::pair<int, NodeT*>> depth_pointer_pairs;
        for(NodeT* p : last_elements)
        {
            depth_pointer_pairs.push_back({tree_nodes[p].i,p});
        }
        std::sort(depth_pointer_pairs.rbegin(), depth_pointer_pairs.rend());

        int pj = 0;
        for(auto pp : depth_pointer_pairs)
        {
            typedef std::pair<int, NodeT*> Qt;
            std::priority_queue<Qt, std::vector<Qt>, std::greater<Qt>> queue;
            queue.push({pj, pp.second});
            int pj_max = 0;
            while(!queue.empty())
            {
                
                NodeT* p = queue.top().second;
                int pjp = queue.top().first;
                queue.pop();
                tree_nodes[p].j = pjp;
                if(p->nast)
                    tree_nodes[p].i =tree_nodes[p->nast].i-1;
                
                int pred_j = pjp;
                for(auto &pred : tree_nodes[p].predecessors)
                {
                    pred.j = pred_j;
                    if(pred.from!=0)
                        queue.push({pred.j, pred.from});
                    pred_j++;
                }
                // p = tree_nodes[p].predecessors.front().p;
                pj_max = pred_j>pj_max?pred_j:pj_max;
            }
            pj = pj_max;
            nr = pj_max>nr?pj_max:nr;
        }
                
    }

    int data_length = 0;

    for(auto pp : tree_nodes)
    {
        NodeT* p = pp.first;
        std::ostringstream str;
        str << p->dane;
        if(str.str().size()>data_length)
            data_length = str.str().size();
    }

            
    std::ostringstream out;
    int char_length = 7;
    int dataWidth = 30 + data_length*char_length;    // width for data
    int nodeHeight = 40;
    int spacing = 70;      // space between nodes
    int height = (nr+null_pointers.size())*(nodeHeight + 20);
    int width = (nc+2) * (dataWidth + spacing);

    out << "<svg xmlns='http://www.w3.org/2000/svg' "
        << "width='" << width << "' height='" << height << "'>\n";
   
    // Arrow marker
    out << "  <defs>\n"
        << "    <marker id='sll_arrow' viewBox='0 0 10 10' refX='9' refY='5' "
        << "markerWidth='6' markerHeight='6' orient='auto-start-reverse'>\n"
        << "      <path d='M 0 0 L 10 5 L 0 10 z' fill='black'/>\n"
        << "    </marker>\n"
        << "  </defs>\n\n";

    int startX = 0; // leave room for glowa arrow
    int startY = 10;

    for(auto pp : tree_nodes)
    {
        NodeT* p = pp.first;
        int pi = pp.second.i;
        int pj = pp.second.j;
        int x = startX + spacing + (dataWidth + spacing)*pi;
        int y = startY + (nodeHeight + 20)*pj;
        drawNode(out, x, y, dataWidth, nodeHeight, p);
        if(!p->nast)
            drawArrow(out, x + dataWidth, x + dataWidth + spacing, y + nodeHeight/2, p->nast, "nast");

        int x_arrow = startX + (dataWidth + spacing)*pi;
        for(auto pred : pp.second.predecessors)
        {
            if(pj== pred.j)
            {
                drawArrow(out, x_arrow, x_arrow + spacing, y + nodeHeight/2, p, pred.label);
            }
            else
            {
                int y_arrow = startY + (nodeHeight + 20)*pred.j;
                int y_arrow_parent = startY + (nodeHeight + 20)*pj;
                drawPath(out, {{x_arrow, y_arrow + nodeHeight/2},
                            {x_arrow + spacing + dataWidth/2, y_arrow + nodeHeight/2},
                            {x_arrow + spacing + dataWidth/2, y_arrow_parent + nodeHeight}}, p, pred.label);
            }
        }
            
    }

    int pj = nr;
    for(auto pp : null_pointers)
    {
        int x_arrow = startX;
        int y = startY + (nodeHeight + 20)*pj;
        drawArrow<NodeT>(out, x_arrow, x_arrow + spacing, y + nodeHeight/2, nullptr, pp.second);
        pj++;
    }

    // // Print tree for DEBUGGING
    // for(auto pp : tree_nodes)
    // {
    //     std::cout << pp.first 
    //         << " " << pp.second.p 
    //         << " " << pp.second.i 
    //         << " " << pp.second.j;
            
    //     for(auto pred : pp.second.predecessors)
    //         std::cout << " (" << pred.p 
    //             << ", " << pred.j
    //             << ", " << pred.label
    //             << ")";
        
    //     if(pp.second.p)
    //         std::cout << " " << pp.second.p->dane;
        
    //     std::cout << std::endl;
    // }

    // for(auto p : last_elements)
    // {
    //     std::cout << p << " ";
    // }
    // std::cout << std::endl;

    // for(auto pp : null_pointers)
    // {
    //     std::cout << pp.second << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "deepest: " << deepest_node << std::endl;

    return out.str();
}

template <typename NodeT>
std::string drawList(std::vector<std::pair<NodeT*, std::string>> pointers, std::string id="", int delay=500)
{    
    std::string svg_source = generateLinkedListSVG(pointers);
    
    ht::html rect(svg_source);
    bool update = !id.empty();
    if(id.empty())
        id = "draw_list_" +  std::to_string(std::rand());
    
    xcpp::display(rect, id.c_str(), update);

    if(delay>0)
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    
    return id;
}

void DRAW_LIST_DEMO()
{
    Node<int>* n3 = new Node<int>(30);
    Node<int>* n2 = new Node<int>(20, n3);
    Node<int>* glowa = new Node<int>(10, n2);

    DRAW_LIST(glowa);

    delete glowa; delete n2; delete n3;
}

void drawList_TEST_1()
{
    Node<int>* n3 = new Node<int>(3000);
    Node<int>* n2 = new Node<int>(20, n3);
    Node<int>* n1 = new Node<int>(10, n2);
    Node<int>* n4 = new Node<int>(30, new Node<int>(40, new Node<int>(50, n3)));
    Node<int>* n5 = n3;
    
    Node<int>* glowa = n1;
    
    Node<int>* nx = 0;
    
    DRAW_LIST(glowa, n1, n2, n3, n4, n5, nx);
    
    delete n1; delete n2; delete n3; delete n4->nast->nast; delete n4->nast; delete n4;
}

void drawList_TEST_2()
{
    Node<int>* n3 = new Node<int>(3000);
    Node<int>* n2 = new Node<int>(20, n3);
    Node<int>* n1 = new Node<int>(10, n2);
    Node<int>* n4 = new Node<int>(40, new Node<int>(50));
    Node<int>* n5 = n3;
    
    Node<int>* glowa = n1;
    
    Node<int>* nx = 0;
    
    DRAW_LIST(glowa, n1, n2, n3, n4, n5, nx);
    
    delete n1; delete n2; delete n3; delete n4->nast; delete n4;
}

#endif