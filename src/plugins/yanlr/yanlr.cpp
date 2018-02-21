/**
 * @file
 *
 * @brief Source for yanlr plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include "yanlr.hpp"
#include "YAMLLexer.h"
#include "YAMLParser.h"
#include "antlr4-runtime.h"
#include "listener.hpp"

#include <iostream>
#include <kdb.hpp>
using namespace ckdb;
#include <kdberrors.h>
#include <kdbhelper.h>

using CppKey = kdb::Key;

using antlr::YAMLLexer;
using antlr::YAMLParser;

using antlr4::ANTLRInputStream;
using antlr4::CommonTokenStream;
using ParseTree = antlr4::tree::ParseTree;
using ParseTreeWalker = antlr4::tree::ParseTreeWalker;

using std::ifstream;

namespace
{
/**
 * @brief This function returns a key set containing the contract of this plugin.
 *
 * @return A contract describing the functionality of this plugin.
 */
KeySet * contractYanlr (void)
{
	return ksNew (30, keyNew ("system/elektra/modules/yanlr", KEY_VALUE, "yanlr plugin waits for your orders", KEY_END),
		      keyNew ("system/elektra/modules/yanlr/exports", KEY_END),
		      keyNew ("system/elektra/modules/yanlr/exports/get", KEY_FUNC, elektraYanlrGet, KEY_END),
		      keyNew ("system/elektra/modules/yanlr/exports/set", KEY_FUNC, elektraYanlrSet, KEY_END),
#include ELEKTRA_README (yanlr)
		      keyNew ("system/elektra/modules/yanlr/infos/version", KEY_VALUE, PLUGINVERSION, KEY_END), KS_END);
}
} // end namespace

extern "C" {
// ====================
// = Plugin Interface =
// ====================

/** @see elektraDocGet */
int elektraYanlrGet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	auto parent = CppKey (parentKey);

	if (parent.getName () == "system/elektra/modules/yanlr")
	{
		KeySet * contract = contractYanlr ();
		ksAppend (returned, contract);
		ksDel (contract);
		parent.release ();

		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}

	ifstream file (parent.getString ());
	if (!file.is_open ())
	{
		ELEKTRA_SET_ERRORF (ELEKTRA_ERROR_COULD_NOT_OPEN, parent.getKey (), "Unable to open file “%s”",
				    parent.getString ().c_str ());
	}

	ANTLRInputStream input (file);
	YAMLLexer lexer (&input);
	CommonTokenStream tokens (&lexer);
	YAMLParser parser (&tokens);
	ParseTreeWalker walker{};
	KeyListener listener{ parent };

	ParseTree * tree = parser.mappings ();
	walker.walk (&listener, tree);

	parent.release ();
	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

/** @see elektraDocSet */
int elektraYanlrSet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned ELEKTRA_UNUSED, Key * parentKey ELEKTRA_UNUSED)
{
	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

Plugin * ELEKTRA_PLUGIN_EXPORT (yanlr)
{
	return elektraPluginExport ("yanlr", ELEKTRA_PLUGIN_GET, &elektraYanlrGet, ELEKTRA_PLUGIN_SET, &elektraYanlrSet,
				    ELEKTRA_PLUGIN_END);
}

} // end extern "C"
