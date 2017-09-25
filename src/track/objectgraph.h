#ifndef OBJECTGRAPH_H
#define OBJECTGRAPH_H

#include <vector>
#include <unordered_map>

class ObjectNode {
public:
    ObjectNode()
    : children(), objectId(nullptr), classId(nullptr), visited(false)
    {
    }

    std::vector<ObjectNode*> children;
    void *objectId;
    void *classId;
    bool visited;

    void print(size_t gcCounter, FILE *out) {
        fprintf(out, "e %zx %zx %zx %zx\n", gcCounter, children.size(), reinterpret_cast<uintptr_t>(objectId),  reinterpret_cast<uintptr_t>(classId));
        for (ObjectNode* child: children) {
            child->print(gcCounter, out);
        }
    }
};

class ObjectGraph {
public:
    ObjectGraph() {}

    void addRoot(void* rootObjectID, void *rootClassID) {
        index(nullptr, nullptr, rootObjectID, rootClassID);
    }

    void index(void *keyObjectID, void *keyClassID, void *valObjectID, void *valClassID) {
        auto keyIt = m_graph.find(keyObjectID);
        if (keyIt == m_graph.end()) {
            ObjectNode node;
            node.objectId = keyObjectID;
            node.classId = keyClassID;
            keyIt = m_graph.insert(std::make_pair(keyObjectID, node)).first;
        }

        auto valIt = m_graph.find(valObjectID);
        if (valIt == m_graph.end()) {
            ObjectNode node;
            node.objectId = valObjectID;
            node.classId = valClassID;
            valIt = m_graph.insert(std::make_pair(valObjectID, node)).first;
        }

        keyIt->second.children.push_back(&(valIt->second));
    }

    void eliminateLoops(ObjectNode *node) {
        node->visited = true;
        node->children.erase(remove_if(node->children.begin(), node->children.end(), [](const ObjectNode* child) -> bool {
            return child->visited;
        }), node->children.end());

        for (size_t i = 0; i < node->children.size(); ++i) {
            eliminateLoops(node->children[i]);
        }
    }

    void print(size_t gcCounter, FILE* out) {
        auto it = m_graph.find(nullptr);
        if (it == m_graph.end()) {
            return;
        }

        ObjectNode *node = &(it->second);
        eliminateLoops(node);
        node->print(gcCounter, out);
    }

    void clear() {
        m_graph.clear();
    }

    /* m_graph is in fact a flat collection of ObjectNodes. The graph structure
       is provided by ObjectNode::children which point to ObjectNode instances
       living inside m_graph. All instances of ObjectNode are residing
       in m_graph, regardless of their position in the actual graph.
       NULL key is somewhat special - the ObjectNode corresponding to it is a fake
       ObjectNode whose children are GC roots. */
    static std::unordered_map<void*, ObjectNode> m_graph;
};

#endif // OBJECTGRAPH_H
