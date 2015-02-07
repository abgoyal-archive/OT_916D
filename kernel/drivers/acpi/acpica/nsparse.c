


#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acparser.h"
#include "acdispat.h"
#include "actables.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsparse")

acpi_status
acpi_ns_one_complete_parse(u32 pass_number,
			   u32 table_index,
			   struct acpi_namespace_node *start_node)
{
	union acpi_parse_object *parse_root;
	acpi_status status;
       u32 aml_length;
	u8 *aml_start;
	struct acpi_walk_state *walk_state;
	struct acpi_table_header *table;
	acpi_owner_id owner_id;

	ACPI_FUNCTION_TRACE(ns_one_complete_parse);

	status = acpi_tb_get_owner_id(table_index, &owner_id);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Create and init a Root Node */

	parse_root = acpi_ps_create_scope_op();
	if (!parse_root) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	/* Create and initialize a new walk state */

	walk_state = acpi_ds_create_walk_state(owner_id, NULL, NULL, NULL);
	if (!walk_state) {
		acpi_ps_free_op(parse_root);
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	status = acpi_get_table_by_index(table_index, &table);
	if (ACPI_FAILURE(status)) {
		acpi_ds_delete_walk_state(walk_state);
		acpi_ps_free_op(parse_root);
		return_ACPI_STATUS(status);
	}

	/* Table must consist of at least a complete header */

	if (table->length < sizeof(struct acpi_table_header)) {
		status = AE_BAD_HEADER;
	} else {
		aml_start = (u8 *) table + sizeof(struct acpi_table_header);
		aml_length = table->length - sizeof(struct acpi_table_header);
		status = acpi_ds_init_aml_walk(walk_state, parse_root, NULL,
					       aml_start, aml_length, NULL,
					       (u8) pass_number);
	}

	if (ACPI_FAILURE(status)) {
		acpi_ds_delete_walk_state(walk_state);
		goto cleanup;
	}

	/* start_node is the default location to load the table */

	if (start_node && start_node != acpi_gbl_root_node) {
		status =
		    acpi_ds_scope_stack_push(start_node, ACPI_TYPE_METHOD,
					     walk_state);
		if (ACPI_FAILURE(status)) {
			acpi_ds_delete_walk_state(walk_state);
			goto cleanup;
		}
	}

	/* Parse the AML */

	ACPI_DEBUG_PRINT((ACPI_DB_PARSE, "*PARSE* pass %d parse\n",
			  (unsigned)pass_number));
	status = acpi_ps_parse_aml(walk_state);

      cleanup:
	acpi_ps_delete_parse_tree(parse_root);
	return_ACPI_STATUS(status);
}


acpi_status
acpi_ns_parse_table(u32 table_index, struct acpi_namespace_node *start_node)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ns_parse_table);

	/*
	 * AML Parse, pass 1
	 *
	 * In this pass, we load most of the namespace.  Control methods
	 * are not parsed until later.  A parse tree is not created.  Instead,
	 * each Parser Op subtree is deleted when it is finished.  This saves
	 * a great deal of memory, and allows a small cache of parse objects
	 * to service the entire parse.  The second pass of the parse then
	 * performs another complete parse of the AML.
	 */
	ACPI_DEBUG_PRINT((ACPI_DB_PARSE, "**** Start pass 1\n"));
	status = acpi_ns_one_complete_parse(ACPI_IMODE_LOAD_PASS1,
					    table_index, start_node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * AML Parse, pass 2
	 *
	 * In this pass, we resolve forward references and other things
	 * that could not be completed during the first pass.
	 * Another complete parse of the AML is performed, but the
	 * overhead of this is compensated for by the fact that the
	 * parse objects are all cached.
	 */
	ACPI_DEBUG_PRINT((ACPI_DB_PARSE, "**** Start pass 2\n"));
	status = acpi_ns_one_complete_parse(ACPI_IMODE_LOAD_PASS2,
					    table_index, start_node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	return_ACPI_STATUS(status);
}
