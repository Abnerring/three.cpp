// Generated from pinned TSLCore.js FunctionExpression ranges; dev-time only.
constexpr const char* kTSLMethodChainingSource="function ( ...params ) {\r\n\r\n\t\t\t//if ( name === 'toVarIntent' ) return this;\r\n\r\n\t\t\treturn this.isStackNode ? this.addToStack( nodeElement( ...params ) ) : nodeElement( this, ...params );\r\n\r\n\t\t}";
constexpr const char* kTSLMethodChainingAssignSource="function ( ...params ) {\r\n\r\n\t\t\treturn this.isStackNode ? this.assign( params[ 0 ], nodeElement( ...params ) ) : this.assign( nodeElement( this, ...params ) );\r\n\r\n\t\t}";
