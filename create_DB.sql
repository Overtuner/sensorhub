CREATE TABLE Actor
(
  Actor_ID int NOT NULL,
  Actor_Name TEXT,
  Add_Info TEXT,
  Node_ID TEXT NOT NULL,
  Channel int NOT NULL,
  Value float,
  Utime int,
  CONSTRAINT Key2 PRIMARY KEY (Actor_ID),
  CONSTRAINT Relationship5 FOREIGN KEY (Node_ID) REFERENCES Node (Node_ID),
  CONSTRAINT Node_ID FOREIGN KEY (Node_ID) REFERENCES Node (Node_ID)
);
CREATE TABLE Battery
(
  Battery_ID int NOT NULL,
  Battery_name TEXT,
  U_Empty float,
  U_Nominal float,
  Battery_sel_txt TEXT,
  CONSTRAINT Key3 PRIMARY KEY (Battery_ID)
);
CREATE TABLE Job
(
  Job_ID int NOT NULL,
  Job_Name TEXT,
  Add_Info TEXT,
  CONSTRAINT Key6 PRIMARY KEY (Job_ID)
);
CREATE TABLE JobBuffer
(
  Job_ID int,
  Seq int,
  Node_ID TEXT,
  Channel int,
  Type int,
  Value float,
  Sensor_ID int,
  Priority int,
  Utime int DEFAULT (strftime('%s','now'))
);
CREATE TABLE JobStep
(
  Job_ID int NOT NULL,
  Seq int NOT NULL,
  Add_Info TEXT,
  Type int,
  ID int,
  Value float,
  Sensor_ID int,
  Priority int DEFAULT 9,
  CONSTRAINT Key5 PRIMARY KEY (Seq,Job_ID),
  CONSTRAINT Job_ID FOREIGN KEY (Job_ID) REFERENCES Job (Job_ID)
);
CREATE TABLE Node
(
  Node_ID TEXT NOT NULL,
  Node_Name TEXT,
  Add_Info TEXT,
  U_Batt float,
  Sleeptime1 int,
  Sleeptime2 int,
  Sleeptime3 int,
  Sleeptime4 int,
  Radiomode int,
  Battery_ID int NOT NULL, voltagedivider INT,
  CONSTRAINT Key1 PRIMARY KEY (Node_ID),
  CONSTRAINT Battery_ID FOREIGN KEY (Battery_ID) REFERENCES Battery (Battery_ID)
);
CREATE TABLE Schedule
(
  Schedule_ID int NOT NULL,
  Job_ID int,
  Triggered_By TEXT,
  Utime int,
  Interval int,
  Trigger_ID int, Trigger_state TEXT,
  CONSTRAINT Key9 PRIMARY KEY (Schedule_ID),
  CONSTRAINT Job_ID FOREIGN KEY (Job_ID) REFERENCES Job (Job_ID)
);
CREATE TABLE Scheduled_Jobs
(
  Job_ID int NOT NULL,
  CONSTRAINT Key11 PRIMARY KEY (Job_ID),
  CONSTRAINT Job_ID FOREIGN KEY (Job_ID) REFERENCES Job (Job_ID)
);
CREATE TABLE Sensor
(
  Sensor_ID int NOT NULL,
  Sensor_Name TEXT,
  Add_Info TEXT,
  Node_ID TEXT NOT NULL,
  Channel int NOT NULL,
  Value float,
  Utime int,
  CONSTRAINT Key2 PRIMARY KEY (Sensor_ID),
  CONSTRAINT Relationship6 FOREIGN KEY (Node_ID) REFERENCES Node (Node_ID),
  CONSTRAINT Node_ID FOREIGN KEY (Node_ID) REFERENCES Node (Node_ID)
);
CREATE TABLE Trigger
(
  Trigger_ID int NOT NULL,
  Trigger_Name TEXT,
  Add_Info TEXT,
  Level_Set float,
  Level_Reset float,
  State TEXT,
  Sensor_ID int,
  CONSTRAINT Key10 PRIMARY KEY (Trigger_ID),
  CONSTRAINT Sensor_ID FOREIGN KEY (Sensor_ID) REFERENCES Sensor (Sensor_ID)
);
CREATE TABLE Triggerdata
(
  Trigger_ID int NOT NULL,
  utime int NOT NULL DEFAULT (strftime('%s','now')),
  state TEXT,
  CONSTRAINT Key13 PRIMARY KEY (Trigger_ID,utime),
  CONSTRAINT Relationship7 FOREIGN KEY (Trigger_ID) REFERENCES Trigger (Trigger_ID)
);
CREATE TABLE sensordata
(
  Sensor_ID int NOT NULL,
  Utime int NOT NULL,
  Value float,
  CONSTRAINT Key4 PRIMARY KEY (Utime,Sensor_ID),
  CONSTRAINT Sensor_ID FOREIGN KEY (Sensor_ID) REFERENCES Sensor (Sensor_ID)
);
CREATE TABLE setactor_ext
(
  Node_ID TEXT,
  Channel int,
  Value float
);
CREATE TABLE test(utime,interval);
CREATE TABLE test_trigger(test int, val int);
CREATE VIEW Actor_HR AS
SELECT Actor_ID, Actor_Name, Add_Info, Node_ID, Channel, Value,  strftime('%d.%m.%Y %H:%M',datetime(utime, 'unixepoch', 'localtime')) AS TimeStamp
FROM Actor;
CREATE VIEW Sensor_HR AS
SELECT Sensor_ID, Sensor_Name, Add_Info, Node_ID, Channel, Value,  strftime('%d.%m.%Y %H:%M',datetime(utime, 'unixepoch', 'localtime')) AS TimeStamp
FROM Sensor;
CREATE VIEW jobbuffer2order
 as 
select a.job_id, a.seq, a.node_id, a.channel, b.value, a.priority  
  from jobbuffer a, sensor b 
 where a.type = 3 and a.sensor_id = b.sensor_id
union all 
select a.job_id, a.seq, a.node_id, a.channel, a.value, a.priority  
  from jobbuffer a 
 where a.type = 2
union all
select a.job_id, a.seq, a.node_id, a.channel, 0 as value, a.priority  
  from jobbuffer a 
 where a.type = 1;
CREATE VIEW Job2Jobbuffer
 AS 
       SELECT a.job_ID, a.seq, b.node_ID, b.channel, a.type, a.value, a.sensor_id, a.priority 
         from jobstep a, actor b
        where (a.type = 2 or a.type=3) and a.id = b.actor_ID  
          and a.job_ID in ( select job_ID from Scheduled_Jobs )
       union all
       SELECT a.job_ID, a.seq, b.node_ID, b.channel, a.type, a.value, a.sensor_id, a.priority 
         from jobstep a, sensor b
        where a.type = 1 and a.id = b.sensor_ID and a.job_ID in ( select job_ID from Scheduled_Jobs );
CREATE VIEW Sensordata_HR AS
SELECT sensordata.Sensor_ID, Sensor_Name, sensordata.Value,  strftime('%d.%m.%Y %H:%M',datetime(sensordata.utime, 'unixepoch', 'localtime')) AS TimeStamp,  strftime('%d.%m.%Y',datetime(sensordata.utime, 'unixepoch', 'localtime')) AS Date, sensordata.Utime
FROM Sensor, sensordata
WHERE sensor.sensor_ID = sensordata.sensor_ID;
CREATE VIEW Sensors_and_actors
 AS 
select sensor_id as id, sensor_name as name, add_info, node_id, channel, Value, Utime, 's' as source
from sensor
union all
select actor_id as id, actor_name as name, add_info, node_id, channel, Value, Utime, 'a' as source
from actor;
CREATE VIEW Jobs4Joblist
 AS
select a.Job_ID, a.Job_Name, b.Seq, 'Sensor abfragen' as Jobtyp, c.name, 0 as value 
from Job a, Jobstep b, sensors_and_actors c
where a.Job_ID = b.Job_ID and c.ID = b.ID and b.type = 1 and c.source = 's'
union all
select a.Job_ID, a.Job_Name, b.Seq, 'Actor auf festen Wert setzen' as Jobtyp, c.name, b.value 
from Job a, Jobstep b, sensors_and_actors c
where a.Job_ID = b.Job_ID and c.ID = b.ID and b.type = 2 and c.source = 'a'
union all
select a.Job_ID, a.Job_Name, b.Seq, 'Actor auf Sensorwert setzen' as Jobtyp, c.name, d.value 
from Job a, Jobstep b, sensors_and_actors c, sensors_and_actors d
where a.Job_ID = b.Job_ID and c.ID = b.ID and c.source = 'a' and d.ID = b.sensor_id and b.type = 3 and d.source = 's';
CREATE VIEW Schedule_HR
 AS 
SELECT Schedule_ID, Job_Name,  strftime('%d.%m.%Y %H:%M:%S',datetime(utime, 'unixepoch', 'localtime')) AS TimeStamp, Interval, Triggered_By, Trigger_ID
FROM Schedule, Job
WHERE schedule.job_id = job.job_id;
CREATE VIEW triggerdata_HR
 AS
SELECT  triggerdata.trigger_ID, 
        trigger_Name, 
        triggerdata.state,
        strftime('%d.%m.%Y %H:%M',datetime(triggerdata.utime, 'unixepoch', 'localtime')) AS TimeStamp,
        strftime('%d.%m.%Y',date(triggerdata.utime, 'unixepoch', 'localtime')) AS Date, triggerdata.Utime
FROM trigger, triggerdata
WHERE trigger.trigger_ID = triggerdata.trigger_ID;
CREATE VIEW Job_HR AS
SELECT Job_Name, Job.Add_Info, JobStep.Job_ID, Seq, Type, JobStep.Add_Info, Value, ID, Sensor_ID, Priority
FROM Job, JobStep
WHERE job.job_id = jobstep.job_id;
CREATE VIEW openjobs as
select "open in scheduled_jobs: " || job_id from  scheduled_jobs
union all
select "open in jobbuffer: " || job_id || " " || seq || " " || node_id || " " || channel from jobbuffer;
CREATE INDEX sensordata_utime on sensordata(Sensor_ID,utime);
CREATE TRIGGER setactor_trigger after insert on setactor_ext
begin
insert into jobbuffer(job_id,seq,node_id,channel,type,value,Sensor_ID,Priority) 
select (select ifnull(max(job_id),101)+1 from jobbuffer where job_id >100 and job_id < 1000),1, node_id, channel, 2, value,0,5 from  setactor_ext;
delete from  setactor_ext;
end;
