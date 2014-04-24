DROP DATABASE IF EXISTS dtnmp;
CREATE DATABASE dtnmp;
USE dtnmp;

DROP TABLE IF EXISTS dbtOutgoing;
CREATE TABLE dbtOutgoing (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  CreatedTS Datetime,
  ModifiedTS Datetime,
  State TINYINT NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtOutgoingMessages;
CREATE TABLE dbtOutgoingMessages (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  OutgoingID INT UNSIGNED NOT NULL DEFAULT 0,
  MessagesTableID INT UNSIGNED NOT NULL DEFAULT 0,
  MessagesTableRowID INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtOutgoingRecipients;
CREATE TABLE dbtOutgoingRecipients (
  OutgoingID INT UNSIGNED NOT NULL DEFAULT 0,
  AgentID INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtIncoming;
CREATE TABLE dbtIncoming (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ReceivedTS Datetime,
  GeneratedTS Datetime,
  State TINYINT NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtIncomingMessages;
CREATE TABLE dbtIncomingMessages (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  IncomingID INT UNSIGNED NOT NULL DEFAULT 0,
  Content BLOB) ;
  
DROP TABLE IF EXISTS lvtMessageTablesList;
CREATE TABLE lvtMessageTablesList (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  TableName VARCHAR(64) NOT NULL DEFAULT 'ERROR') ;

DROP TABLE IF EXISTS dbtRegisteredAgents;
CREATE TABLE dbtRegisteredAgents (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  AgentId VARCHAR(128) NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesAdministrativeStatusReportingPolicy;
CREATE TABLE dbtMessagesAdministrativeStatusReportingPolicy (
  AgentID INT UNSIGNED NOT NULL DEFAULT 0 PRIMARY KEY,
  AlertBit Bit(1),
  WarnBit Bit(1),
  ErrorBit Bit(1),
  LogBit Bit(1)) ;

DROP TABLE IF EXISTS dbtMessagesAdministrativeStatusMessages;
CREATE TABLE dbtMessagesAdministrativeStatusMessages (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Code INT UNSIGNED NOT NULL DEFAULT 0,
  Time INT UNSIGNED NOT NULL DEFAULT 0,
  GeneratorsMC INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesDefinitions;
CREATE TABLE dbtMessagesDefinitions (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  DefinitionType TINYINT NOT NULL DEFAULT 0,
  DefinitionID INT UNSIGNED NOT NULL DEFAULT 0,
  DefinitionMC INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesReportingDataList;
CREATE TABLE dbtMessagesReportingDataList (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ConfiguredIdMC INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesReportingDataDefinitions;
CREATE TABLE dbtMessagesReportingDataDefinitions (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  DefinitionID INT UNSIGNED NOT NULL DEFAULT 0,
  DefinitionMC CHAR(64)) ;

DROP TABLE IF EXISTS dbtMessagesReportingDataReport;
CREATE TABLE dbtMessagesReportingDataReport (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  MessageGroupID INT UNSIGNED NOT NULL DEFAULT 0,
  Time INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesReportingDataReportList;
CREATE TABLE dbtMessagesReportingDataReportList (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  DataReportID INT UNSIGNED NOT NULL DEFAULT 0,
  ReportID INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesReportingDataReportData;
CREATE TABLE dbtMessagesReportingDataReportData (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  DataReportListID INT UNSIGNED NOT NULL DEFAULT 0,
  Data TINYINT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesReportingProductionScheduleReport;
CREATE TABLE dbtMessagesReportingProductionScheduleReport (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  MessageGroupID INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesReportingProductionScheduleReports;
CREATE TABLE dbtMessagesReportingProductionScheduleReports (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ReportID INT UNSIGNED NOT NULL DEFAULT 0,
  Type TINYINT UNSIGNED NOT NULL DEFAULT 0,
  Start INT UNSIGNED NOT NULL DEFAULT 0,
  ConditionID INT UNSIGNED NOT NULL DEFAULT 0,
  Count	BIGINT UNSIGNED NOT NULL DEFAULT 0,
  Results INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMessagesControls;
CREATE TABLE dbtMessagesControls (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Type TINYINT UNSIGNED NOT NULL DEFAULT 0,
  Start BIGINT UNSIGNED NOT NULL DEFAULT 0,
  Periods BIGINT UNSIGNED NOT NULL DEFAULT 1,
  Predicate INT UNSIGNED NOT NULL DEFAULT 0,
  Count BIGINT UNSIGNED NOT NULL DEFAULT 0,
  Collection INT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMIDs;
CREATE TABLE dbtMIDs (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Attributes INT UNSIGNED NOT NULL DEFAULT 0,
  Type Bit(2) NOT NULL DEFAULT 0,
  Category Bit(2) NOT NULL DEFAULT 0,
  IssuerFlag Bit(1) NOT NULL DEFAULT 0,
  TagFlag Bit(1) NOT NULL DEFAULT 0,
  OIDType Bit(2) NOT NULL DEFAULT 0,
  IssuerID BIGINT UNSIGNED NOT NULL DEFAULT 0,
  OIDValue VARCHAR(64),
  TagValue BIGINT UNSIGNED NOT NULL DEFAULT 0) ;

DROP TABLE IF EXISTS dbtMIDCollections;
CREATE TABLE dbtMIDCollections (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Comment VARCHAR(255)) ;

DROP TABLE IF EXISTS dbtMIDCollection;
CREATE TABLE dbtMIDCollection (
  CollectionID INT UNSIGNED NOT NULL DEFAULT 0,
  MIDID INT UNSIGNED NOT NULL DEFAULT 0,
  MIDOrder INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`CollectionID`,`MIDID`,`MIDOrder`)) ;

DROP TABLE IF EXISTS dbtMIDDetails;
CREATE TABLE dbtMIDDetails (
  MIDID INT UNSIGNED NOT NULL DEFAULT 0,
  MIBName VARCHAR(50) NOT NULL DEFAULT '',
  MIBISO VARCHAR(255) NOT NULL DEFAULT '',
  Name VARCHAR(50) NOT NULL DEFAULT '',
  Description VARCHAR(255) NOT NULL DEFAULT '') ;

DROP TABLE IF EXISTS lvtMIDAttributes;
CREATE TABLE lvtMIDAttributes (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Name VARCHAR(50) NOT NULL DEFAULT '',
  Description VARCHAR(255) NOT NULL DEFAULT '') ;

DROP TABLE IF EXISTS lvtMIDIssuers;
CREATE TABLE lvtMIDIssuers (
  ID BIGINT UNSIGNED NOT NULL PRIMARY KEY,
  Name VARCHAR(50) NOT NULL DEFAULT 'Unknown Issuer',
  Description VARCHAR(255) NOT NULL DEFAULT 'No Description.') ;

DROP TABLE IF EXISTS dbtMIDParameterizedOIDs;
CREATE TABLE dbtMIDParameterizedOIDs (
  MIDID INT UNSIGNED NOT NULL DEFAULT 0,
  DataCollectionID INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`MIDID`,`DataCollectionID`)) ;

DROP TABLE IF EXISTS dbtDataCollections;
CREATE TABLE dbtDataCollections (
  ID INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Comment VARCHAR(255)) ;

DROP TABLE IF EXISTS dbtDataCollection;
CREATE TABLE dbtDataCollection (
  CollectionID INT UNSIGNED NOT NULL DEFAULT 0,
  DataLength INT UNSIGNED NOT NULL DEFAULT 0,
  DataBlob MEDIUMBLOB ,
  DataOrder INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`CollectionID`,`DataOrder`)) ;

DROP TABLE IF EXISTS lvtOutgoingState;
CREATE TABLE lvtOutgoingState (
  ID INT UNSIGNED NOT NULL PRIMARY KEY,
  Name VARCHAR(50) NOT NULL DEFAULT '',
  Description VARCHAR(255) NOT NULL DEFAULT '') ;

DROP TABLE IF EXISTS lvtIncomingState;
CREATE TABLE lvtIncomingState (
  ID INT UNSIGNED NOT NULL PRIMARY KEY,
  Name VARCHAR(50) NOT NULL DEFAULT '',
  Description VARCHAR(255) NOT NULL DEFAULT '') ;

-- Populate lvtOutgoingState
INSERT INTO lvtOutgoingState (ID,Name,Description) VALUES (0,'Initializing','');
INSERT INTO lvtOutgoingState (ID,Name,Description) VALUES (1,'Ready','');
INSERT INTO lvtOutgoingState (ID,Name,Description) VALUES (2,'Processing','');
INSERT INTO lvtOutgoingState (ID,Name,Description) VALUES (3,'Sent','');

-- Populate lvtIncomingState
INSERT INTO lvtIncomingState (ID,Name,Description) VALUES (0,'Initializing','');
INSERT INTO lvtIncomingState (ID,Name,Description) VALUES (1,'Ready','');
INSERT INTO lvtIncomingState (ID,Name,Description) VALUES (2,'Processed','');

-- Populate lvtMessageTablesList
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtOutgoing');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtOutgoingMessages');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtOutgoingRecipients');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtIncoming');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtIncomingMessages');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtRegisteredAgents');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesAdministrativeStatusReportingPolicy');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesAdministrativeStatusMessages');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesDefinitions');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesReportingDataList');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesReportingDataDefinitions');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesReportingDataReport');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesReportingDataReportList');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesReportingDataReportData');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesReportingProductionScheduleReport');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesReportingProductionScheduleReports');
INSERT INTO lvtMessageTablesList (TableName) VALUES ('dbtMessagesControls');

-- Populate lvtMIDAttributes
INSERT INTO lvtMIDAttributes (Name,Description) VALUES ('Atomic MID','This is an atomic MID.');
INSERT INTO lvtMIDAttributes (Name,Description) VALUES ('User MID','This is an user defined MID.');

-- Populate lvtMIDIssuers
INSERT INTO lvtMIDIssuers (ID,Name,Description) VALUES (1,'Ed','Ed B.');
INSERT INTO lvtMIDIssuers (ID,Name,Description) VALUES (2,'Miriam','Miriam W.');
INSERT INTO lvtMIDIssuers (ID,Name,Description) VALUES (3,'Leor','Leor B.');
INSERT INTO lvtMIDIssuers (ID,Name,Description) VALUES (4,'Mark','Mark S.');
INSERT INTO lvtMIDIssuers (ID,Name,Description) VALUES (5,'Sam','Sam J.');
