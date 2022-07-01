Sky DeBaun  - sky.u.debaun@jpl.nasa.gov


This document highlights modifications made to correct NM Manager database connection errors in ION 4.1.1 (and the related amp-sql-1.0.tar). Persuant to these updates the new amp-sql directory has tentatively been named amp-sql-1.0.1 to reflect the modifications.

The modifications are derived (or cloned directly) from the anms-ion patch and updated sql creation scripts made for ANMS (AMMOS Asynchronous Network Management System) v0.1.1 located at:
https://github.jpl.nasa.gov/MGSS/anms


An overview of changes made to address the connection issue consists of the following:
	- a modified nm_mgr_sql.c (derived from the patch located at anms/ion/anms-ion-4.1.1.patch)
	- updates to the sql creation scripts (cloned from anms/amp-sql/mysql/)


The previously named amp-sql-1.0.tar has been renamed to deprecated-amp-sql-1.0.tar (but left in place).

***Use the updated amp-sql-1.0.1.tar.gz instead***
