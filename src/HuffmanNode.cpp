#include "HuffmanNode.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace std;

HuffmanNode::HuffmanNode(uint8_t val, uint64_t freq)
    : byteValue(val),
      frequency(freq),
      left(nullptr),
      right(nullptr),
      minByteInSubtree(static_cast<uint16_t>(val)) {}

HuffmanNode::HuffmanNode(HuffmanNode* l, HuffmanNode* r)
    : byteValue(0),
      frequency((l ? l->frequency : 0) + (r ? r->frequency : 0)),
      left(l),
      right(r),
      minByteInSubtree(min(l ? l->minByteInSubtree : 0xFFFF,
                           r ? r->minByteInSubtree : 0xFFFF)) {}

HuffmanNode::~HuffmanNode() {
    delete left;
    delete right;
    left = nullptr;
    right = nullptr;
}

bool HuffmanNode::isLeaf() const {
    return left == nullptr && right == nullptr;
}

string HuffmanNode::getDisplayLabel() const {
    ostringstream oss;
    if (isLeaf()) {
        if (byteValue >= 32 && byteValue <= 126) {
            oss << "'" << static_cast<char>(byteValue) << "'";
        } else {
            switch (byteValue) {
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                case 0:    oss << "NUL"; break;
                default:
                    oss << "0x" << hex << uppercase << setw(2) << setfill('0')
                        << static_cast<int>(byteValue);
                    break;
            }
        }
        oss << " (" << dec << frequency << ")";
    } else {
        oss << "\u03A3=" << frequency;
    }
    return oss.str();
}
