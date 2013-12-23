use dtnmp;


-- Create MID Collection
INSERT INTO dbtMIDCollections(Comment) VALUES('m1');
SET @coll = LAST_INSERT_ID();
INSERT INTO dbtMIDCollection(CollectionID,MIDID,MIDOrder) VALUES(@coll,4,1);
INSERT INTO dbtMIDCollection(CollectionID,MIDID,MIDOrder) VALUES(@coll,2,2);
INSERT INTO dbtMIDCollection(CollectionID,MIDID,MIDOrder) VALUES(@coll,3,3);

-- Define a Control
INSERT INTO dbtMessagesControls(Type,Start,Periods,Predicate,Count,Collection) VALUES(1,0,1,0,1,@coll);

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


PREPARE stmt1 FROM @s; 
EXECUTE stmt1; 
DEALLOCATE PREPARE stmt1;
