// Generated from pinned UserDataNode.js; metadata is never executed.
namespace UserDataNodeSource {
inline constexpr const char* UserDataNode="class UserDataNode extends ReferenceNode {\r\n\r\n\tstatic get type() {\r\n\r\n\t\treturn 'UserDataNode';\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Constructs a new user data node.\r\n\t *\r\n\t * @param {string} property - The property name that should be referenced by the node.\r\n\t * @param {string} inputType - The node data type of the reference.\r\n\t * @param {?Object} [userData=null] - A reference to the `userData` object. If not provided, the `userData` property of the 3D object that uses the node material is evaluated.\r\n\t */\r\n\tconstructor( property, inputType, userData = null ) {\r\n\r\n\t\tsuper( property, inputType, userData );\r\n\r\n\t\t/**\r\n\t\t * A reference to the `userData` object. If not provided, the `userData`\r\n\t\t * property of the 3D object that uses the node material is evaluated.\r\n\t\t *\r\n\t\t * @type {?Object}\r\n\t\t * @default null\r\n\t\t */\r\n\t\tthis.userData = userData;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Overwritten to make sure {@link ReferenceNode#reference} points to the correct\r\n\t * `userData` field.\r\n\t *\r\n\t * @param {(NodeFrame|NodeBuilder)} state - The current state to evaluate.\r\n\t * @return {Object} A reference to the `userData` field.\r\n\t */\r\n\tupdateReference( state ) {\r\n\r\n\t\tthis.reference = this.userData !== null ? this.userData : state.object.userData;\r\n\r\n\t\treturn this.reference;\r\n\r\n\t}\r\n\r\n}";
inline constexpr const char* get_type="get type() {\r\n\r\n\t\treturn 'UserDataNode';\r\n\r\n\t}";
inline constexpr const char* updateReference="updateReference( state ) {\r\n\r\n\t\tthis.reference = this.userData !== null ? this.userData : state.object.userData;\r\n\r\n\t\treturn this.reference;\r\n\r\n\t}";
inline constexpr const char* userData="( name, inputType, userData ) => new UserDataNode( name, inputType, userData )";
inline const char* get(const std::string& name){
    if(name=="UserDataNode")return UserDataNode;
    if(name=="get_type")return get_type;
    if(name=="updateReference")return updateReference;
    if(name=="userData")return userData;
    throw std::logic_error("Unknown UserDataNode source metadata");
}
}
