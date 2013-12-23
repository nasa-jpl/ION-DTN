Use dtnmp;
-- Make a Control MID
-- INSERT INTO dbtMIDs(Attributes,Type,Category,IssuerFlag,TagFlag,OIDType,IssuerID,OIDValue,TagValue) VALUES(0,b'01',b'00',b'0',b'0',0,0,'092A030102017A010300',0);

INSERT INTO dbtMIDs(Attributes,Type,Category,IssuerFlag,TagFlag,OIDType,IssuerID,OIDValue,TagValue) VALUES(0,b'01',b'00',b'0',b'0',b'01',0,'082B06010203010301',0);


SET @MIDID := LAST_INSERT_ID();


-- Make a MID Collection
INSERT INTO dbtMIDCollections(Comment) VALUES('m1');
SET @MCID := LAST_INSERT_ID();

INSERT INTO dbtMIDCollection(CollectionID,MIDID,MIDOrder) VALUES(@MCID,@MIDID,1);

-- Define a Control
INSERT INTO dbtMessagesControls(Type,Start,Periods,Predicate,Count,Collection) VALUES(3,0,0,0,0,@MCID);

-- Define Data Collection
INSERT INTO dbtDataCollections(comment) VALUES('c1');
SET @DCID := LAST_INSERT_ID();

-- Define Data Collection Entry
INSERT INTO dbtDataCollection(CollectionID,DataLength,DataBlob,DataOrder) VALUES(@DCID,11,'ipn:21404.1',1);

-- Enter info into ParameterizedOIDs
INSERT INTO dbtMIDParameterizedOIDs(MIDID,DataCollectionID) VALUES(@MIDID,@DCID);   

-- Register an agent
INSERT INTO dbtRegisteredAgents(AgentID) VALUES('ipn:21404.1');

-- Create an outgoing Message Group with the Control from above
INSERT INTO dbtOutgoing(CreatedTS,ModifiedTS,State) VALUES(NOW(),NOW(),0);
SET @v1 = LAST_INSERT_ID();
SELECT @v2 := ID FROM lvtMessageTablesList WHERE TableName='dbtMessagesControls';
SELECT @v3 := MAX(ID) FROM dbtMessagesControls;

INSERT INTO dbtOutgoingMessages(OutgoingID,MessagesTableID,MessagesTableRowID) VALUES(@v1,@v2,@v3);

-- Retrieve unprocessed (READY) Outgoing Messages
SELECT @MessageGroupID := dbtOutgoing.ID, @TableName := lvtMessageTablesList.TableName, @TableRowID := dbtOutgoingMessages.MessagesTableRowID FROM dbtOutgoing, dbtOutgoingMessages, lvtMessageTablesList WHERE dbtOutgoing.State=0 AND dbtOutgoing.ID=dbtOutgoingMessages.OutgoingID AND lvtMessageTablesList.ID=dbtOutgoingMessages.MessagesTableID;

-- Give a sender for the (READY) Outgoing Messages
SELECT @sender := ID FROM dbtOutgoing WHERE dbtOutgoing.State=0;
INSERT INTO dbtOutgoingRecipients(OutgoingID,AgentID) VALUES(@sender,1);

-- Retrieve the contents of the table listed as an Outgoing Message
SET @s = CONCAT('select * from ', @TableName, ' where ID = ', @TableRowID); 
PREPARE stmt1 FROM @s; 
EXECUTE stmt1; 
DEALLOCATE PREPARE stmt1;


-- Tell Manager we are ready to process.
UPDATE dbtOutgoing SET State = State + 1 WHERE ID = @v1;




