# Catalog queries for views
# CAUTION: Do not modify this file unless you know what you are doing.
# Code generation can be broken if incorrect changes are made.

%if {list} %then
	%if {use-signature} %then
		%set {signature} [ ns.nspname || '.' || ]
	%end

	[SELECT vw.oid, vw.relname AS name, ns.nspname AS parent, 'schema' AS parent_type, NULL AS extra_info
	 FROM pg_class AS vw
 	 LEFT JOIN pg_namespace AS ns ON ns.oid=vw.relnamespace 
	 WHERE vw.relkind IN ('v','m') ]

	%if {schema} %then
		[ AND ns.nspname= ] '{schema}'
	%end

	%if {last-sys-oid} %then
		[ AND vw.oid ] {oid-filter-op} $sp {last-sys-oid}
	%end

	%if {not-ext-object} %then
		[ AND ] ( {not-ext-object} )
	%end

	%if {name-filter} %then
		[ AND ] ( {signature} [ vw.relname ~* ] E'{name-filter}' )
	%end
%else
	%if {attribs} %then
		[SELECT vw.oid, vw.relname AS name, vw.relnamespace AS schema, vw.relowner AS owner,
		vw.reltablespace AS tablespace,
		vw.relacl AS permission, pg_get_viewdef(vw.oid) AS definition, ]

		# Subquery that retrieves the tables that the view refereces
		# Extracted from the definition of the relation information_schema.view_table_usage
		[(SELECT array_agg(DISTINCT dt.refobjid)
		FROM pg_namespace AS nv,
		pg_class AS v,
		pg_depend AS dv,
		pg_depend AS dt,
		pg_class AS t,
		pg_namespace AS nt
		WHERE nv.oid = v.relnamespace AND v.relkind IN ('v','m')
		AND v.oid = dv.refobjid AND dv.refclassid = 'pg_class'::regclass::oid
		AND dv.classid = 'pg_rewrite'::regclass::oid AND dv.deptype = 'i'
		AND dv.objid = dt.objid AND dv.refobjid <> dt.refobjid AND dt.classid = 'pg_rewrite'::regclass::oid
		AND dt.refclassid = 'pg_class'::regclass::oid AND dt.refobjid = t.oid
		AND t.relnamespace = nt.oid AND t.relkind IN ('r', 'p', 'f')
		AND v.oid = ] vw.oid [) AS ref_tables, ]

		[ CASE
		WHEN vw.relkind='m' THEN TRUE
		ELSE FALSE
		END AS materialized_bool, ]

		({comment}) [ AS comment ]

		[ FROM pg_class AS vw
		  LEFT JOIN pg_namespace AS ns ON ns.oid = vw.relnamespace 
	      WHERE vw.relkind IN ('v','m') ]

		%if {last-sys-oid} %then
			[ AND vw.oid ] {oid-filter-op} $sp {last-sys-oid}
		%end

		%if {not-ext-object} %then
			[ AND ] ( {not-ext-object} )
		%end

		%if {filter-oids} %or {schema} %then
			[ AND ]

			%if {filter-oids} %then
				[ vw.oid IN (] {filter-oids} )

				%if {schema} %then
					[ AND ]
				%end
			%end

			%if {schema} %then
				[ ns.nspname= ] '{schema}'
			%end
		%end
	%end
%end
