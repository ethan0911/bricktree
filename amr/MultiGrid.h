#if 0
      struct NodeRef {
        union {
          struct {
            uint32 childID:24;
            uint32 mustBe1:8;
          };
          int32 intBits;
          float value;
        };
        NodeRef(float f=0.f) : value(f) { assert(isLeaf()); }
        NodeRef(uint32 ui) : childID(ui), mustBe1(0xff) { assert(ui == childID); assert(!isLeaf()); }

        inline bool isLeaf() const { return (intBits >> 24) != -1; }
      };
      
      struct Node
      {
        NodeRef childRef[2][2][2];
      };
      
      
      Range<float> rootRange;
      NodeRef      rootRef;
      
      std::vector<Node>          node;
      std::vector<Range<float> > range;

      static inline bool isLeaf(NodeRef ref) { return ref.isLeaf(); }
      inline float leafValue(NodeRef ref)    { return ref.value; }
      inline uint32 nodeID(NodeRef ref)      { return ref.childID; }
#endif
