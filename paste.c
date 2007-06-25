#define NDR_RQST_FUNCTION(name) \
int #name(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep) \
{ \
        struct pidl_pull *ndr = pidl_pull_init(tvb, offset, pinfo, drep); \
        struct #name *r = talloc_p(NULL, struct #name); \
        pidl_tree ptree; \
        ptree.proto_tree = tree; \
        ptree.subtree_list = NULL; \
        ndr_pull_#name(ndr, NDR_IN, &ptree, r); \
        return ndr->offset; \
}
