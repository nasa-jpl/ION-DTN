Use dtnmp;

-- Create MID Collection
INSERT INTO dbtMIDCollections(Comment) VALUES('m1');
SET @coll = LAST_INSERT_ID();
INSERT INTO dbtMIDCollection(CollectionID,MIDID,MIDOrder) VALUES(@coll,4,1);
INSERT INTO dbtMIDCollection(CollectionID,MIDID,MIDOrder) VALUES(@coll,2,2);
INSERT INTO dbtMIDCollection(CollectionID,MIDID,MIDOrder) VALUES(@coll,3,3);

-- Define a Control
INSERT INTO dbtMessagesControls(Type,Start,Periods,Predicate,Count,Collection) VALUES(1,0,1,0,3,@coll);

-- Create an outgoing Message Group with the Control from above
INSERT INTO dbtOutgoing(CreatedTS,ModifiedTS,State) VALUES(NOW(),NOW(),0);
SET @v1 = LAST_INSERT_ID();
SELECT @v2 := ID FROM lvtMessageTablesList WHERE TableName='dbtMessagesControls';
SELECT @v3 := MAX(ID) FROM dbtMessagesControls;

INSERT INTO dbtOutgoingMessages(OutgoingID,MessagesTableID,MessagesTableRowID) VALUES(@v1,@v2,@v3);
UPDATE dbtOutgoing SET State = State + 1 WHERE ID = @v1;

-- Retrieve unprocessed (READY) Outgoing Messages
SELECT @MessageGroupID := dbtOutgoing.ID, @TableName := lvtMessageTablesList.TableName, @TableRowID := dbtOutgoingMessages.MessagesTableRowID FROM dbtOutgoing, dbtOutgoingMessages, lvtMessageTablesList WHERE dbtOutgoing.State=1 AND dbtOutgoing.ID=dbtOutgoingMessages.OutgoingID AND lvtMessageTablesList.ID=dbtOutgoingMessages.MessagesTableID;

-- Retrieve the contents of the table listed as an Outgoing Message
SET @s = CONCAT('select * from ', @TableName, ' where ID = ', @TableRowID); 
PREPARE stmt1 FROM @s; 
EXECUTE stmt1; 
DEALLOCATE PREPARE stmt1;

-- Create Report #1
-- Get MIDs
SET @MID1 := (SELECT ID FROM dbtMIDs WHERE ID = 5);

-- Create Report MID
INSERT INTO dbtMIDs (Attributes,Type,Category,IssuerFlag,TagFlag,OIDType,IssuerID,OIDValue,TagValue) VALUES (2,0,2,false,false,0,2,'92a0301020100010403',0);
SET @DefineCustomReportMIDIDValue = LAST_INSERT_ID();
-- Create MID Collection ID
INSERT INTO dbtMIDCollections (Comment) VALUES ('R1');
SET @MIDCollectionID = LAST_INSERT_ID();
-- Create MID Collection
INSERT INTO dbtMIDCollection (CollectionID,MIDID,MIDOrder) VALUES (@MIDCollectionID,@MID1,1);

-- Create Definition for Define Custom Report
INSERT INTO dbtMessagesDefinitions (DefinitionType,DefinitionID,DefinitionMC) VALUES (1,@DefineCustomReportMIDIDValue,@MIDCollectionID);

-- Define a Control
INSERT INTO dbtMessagesControls(Type,Start,Periods,Predicate,Count,Collection) VALUES(1,1,2,0,3,@MIDCollectionID);

-- Create an outgoing Message Group with the Control from above
INSERT INTO dbtOutgoing(CreatedTS,ModifiedTS,State) VALUES(NOW(),NOW(),0);
SET @v1 = LAST_INSERT_ID();
SELECT @v2 := ID FROM lvtMessageTablesList WHERE TableName='dbtMessagesControls';
SELECT @v3 := MAX(ID) FROM dbtMessagesControls;

INSERT INTO dbtOutgoingMessages(OutgoingID,MessagesTableID,MessagesTableRowID) VALUES(@v1,@v2,@v3);
UPDATE dbtOutgoing SET State = State + 1 WHERE ID = @v1;

-- Retrieve unprocessed (READY) Outgoing Messages
SELECT @MessageGroupID := dbtOutgoing.ID, @TableName := lvtMessageTablesList.TableName, @TableRowID := dbtOutgoingMessages.MessagesTableRowID FROM dbtOutgoing, dbtOutgoingMessages, lvtMessageTablesList WHERE dbtOutgoing.State=1 AND dbtOutgoing.ID=dbtOutgoingMessages.OutgoingID AND lvtMessageTablesList.ID=dbtOutgoingMessages.MessagesTableID;


