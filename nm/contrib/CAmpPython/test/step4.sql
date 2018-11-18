-- Title: DTN Management Protocol database schema
-- Description: Database schema for DTN Management Protocol based on
--              the Internet-Draft "Delay Tolerant Network Management Protocol
--              draft-irtf-dtnrg-dtnmp-01"
--              (https://tools.ietf.org/html/draft-irtf-dtnrg-dtnmp-01).
-- Conventions: 
--             "lvt" = "Limited Value Table": This table is intended to have a
--              set of known values that are not intended to be
--              modified by users on a regular basis.
--             "dbt" = "DataBase Table": This table is intended for almost all
                conventional database transactions a user, application,
                or system may participate in on a regular basis.


USE dtnmp_app;


-- Insert Agent ADM
CALL sp_create_adm_root_step1("iso.identified-organization.dod.internet.mgmt.dtnmp.agent", "1.3.6.1.2.3.3', '2B0601020303", "Agent ADM", @OIDID);
CALL sp_create_adm_root_step2(@OIDID, "DTNMP Agent ADM", "2016-06-29",@ADMID);


-- Insert Agent Nicknames
CALL sp_create_adm_nickname(@ADMID, 0, "Agent Metadata", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 1, "Agent Externally Defined Data", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 2, "Agent Variable Defs", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 3, "Agent Reports", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 4, "Agent Controls", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 5, "Agent Constants", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 6, "Agent Macros", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 7, "Agent Operators", @OIDID, @NNID);
CALL sp_create_adm_nickname(@ADMID, 8, "Agent Root", @OIDID, @NNID);


-- Insert Agent ADM Metadata
