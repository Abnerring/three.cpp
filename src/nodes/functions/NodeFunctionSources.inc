// Generated class and member source positions for the node materials port.
#pragma once
namespace ShadowMaskModelSource {
constexpr const char* classSource="class ShadowMaskModel extends LightingModel {\r\n\r\n	/**\r\n	 * Constructs a new shadow mask model.\r\n	 */\r\n	constructor() {\r\n\r\n		super();\r\n\r\n		/**\r\n		 * The shadow mask node.\r\n		 *\r\n		 * @type {Node}\r\n		 */\r\n		this.shadowNode = float( 1 ).toVar( 'shadowMask' );\r\n\r\n	}\r\n\r\n	/**\r\n	 * Only used to save the shadow mask.\r\n	 *\r\n	 * @param {Object} input - The input data.\r\n	 */\r\n	direct( { lightNode } ) {\r\n\r\n		if ( lightNode.shadowNode !== null ) {\r\n\r\n			this.shadowNode.mulAssign( lightNode.shadowNode );\r\n\r\n		}\r\n\r\n	}\r\n\r\n	/**\r\n	 * Uses the shadow mask to produce the final color.\r\n	 *\r\n	 * @param {NodeBuilder} builder - The current node builder.\r\n	 */\r\n	finish( { context } ) {\r\n\r\n		diffuseColor.a.mulAssign( this.shadowNode.oneMinus() );\r\n\r\n		context.outgoingLight.rgb.assign( diffuseColor.rgb ); // TODO: Optimize LightsNode to avoid this assignment\r\n\r\n	}\r\n\r\n}";
constexpr const char* constructorSource="constructor() {\r\n\r\n		super();\r\n\r\n		/**\r\n		 * The shadow mask node.\r\n		 *\r\n		 * @type {Node}\r\n		 */\r\n		this.shadowNode = float( 1 ).toVar( 'shadowMask' );\r\n\r\n	}";
constexpr const char* directSource="direct( { lightNode }";
constexpr const char* finishSource="finish( { context }";
}
