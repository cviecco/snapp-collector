-- MySQL dump 10.11
--
-- Host: localhost    Database: snapp_i2
-- ------------------------------------------------------
-- Server version	5.0.77

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `snapp_i2`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `snapp_i2` /*!40100 DEFAULT CHARACTER SET latin1 */;


--
-- Table structure for table `acl_role`
--

DROP TABLE IF EXISTS `acl_role`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `acl_role` (
  `acl_role_id` int(10) NOT NULL,
  `name` varchar(30) NOT NULL,
  PRIMARY KEY  (`acl_role_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `acl_role`
--

LOCK TABLES `acl_role` WRITE;
/*!40000 ALTER TABLE `acl_role` DISABLE KEYS */;
INSERT INTO `acl_role` VALUES (0,'Automated');
/*!40000 ALTER TABLE `acl_role` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `category`
--

DROP TABLE IF EXISTS `category`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `category` (
  `category_id` int(10) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `description` varchar(128) default NULL,
  PRIMARY KEY  (`category_id`),
  UNIQUE KEY `name_idx` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=1144 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `category`
--

LOCK TABLES `category` WRITE;
/*!40000 ALTER TABLE `category` DISABLE KEYS */;
INSERT INTO `category` VALUES (1,'Custom','Custom');
/*!40000 ALTER TABLE `category` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `category_acl`
--

DROP TABLE IF EXISTS `category_acl`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `category_acl` (
  `acl_role_id` int(10) NOT NULL,
  `category_id` int(10) NOT NULL,
  `can_edit` tinyint(10) NOT NULL,
  PRIMARY KEY  (`acl_role_id`,`category_id`),
  KEY `category_category_acl_fk` (`category_id`),
  CONSTRAINT `category_category_acl_fk` FOREIGN KEY (`category_id`) REFERENCES `category` (`category_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `user_role_category_acl_fk` FOREIGN KEY (`acl_role_id`) REFERENCES `acl_role` (`acl_role_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `category_acl`
--

LOCK TABLES `category_acl` WRITE;
/*!40000 ALTER TABLE `category_acl` DISABLE KEYS */;
/*!40000 ALTER TABLE `category_acl` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `category_collection_membership`
--

DROP TABLE IF EXISTS `category_collection_membership`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `category_collection_membership` (
  `collection_id` int(8) NOT NULL,
  `category_id` int(10) NOT NULL,
  `end_epoch` int(10) NOT NULL,
  `start_epoch` int(10) NOT NULL,
  PRIMARY KEY  (`collection_id`,`category_id`,`end_epoch`),
  KEY `category_category_collection_mapping_fk` (`category_id`),
  CONSTRAINT `category_category_collection_mapping_fk` FOREIGN KEY (`category_id`) REFERENCES `category` (`category_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `collection_category_collection_mapping_fk` FOREIGN KEY (`collection_id`) REFERENCES `collection` (`collection_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `category_collection_membership`
--

LOCK TABLES `category_collection_membership` WRITE;
/*!40000 ALTER TABLE `category_collection_membership` DISABLE KEYS */;
/*!40000 ALTER TABLE `category_collection_membership` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `category_hierarchy`
--

DROP TABLE IF EXISTS `category_hierarchy`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `category_hierarchy` (
  `parent_category_id` int(10) NOT NULL,
  `child_category_id` int(10) NOT NULL,
  `end_epoch` int(10) NOT NULL,
  `start_epoch` int(10) NOT NULL,
  PRIMARY KEY  (`parent_category_id`,`child_category_id`,`end_epoch`),
  KEY `category_cateogory_hiearchy_fk_1` (`child_category_id`),
  CONSTRAINT `category_cateogory_hiearchy_fk` FOREIGN KEY (`parent_category_id`) REFERENCES `category` (`category_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `category_cateogory_hiearchy_fk_1` FOREIGN KEY (`child_category_id`) REFERENCES `category` (`category_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `category_hierarchy`
--

LOCK TABLES `category_hierarchy` WRITE;
/*!40000 ALTER TABLE `category_hierarchy` DISABLE KEYS */;
/*!40000 ALTER TABLE `category_hierarchy` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `collection`
--

DROP TABLE IF EXISTS `collection`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `collection` (
  `collection_id` int(8) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `host_id` int(8) NOT NULL,
  `rrdfile` varchar(512) default NULL,
  `premap_oid_suffix` varchar(64) NOT NULL,
  `collection_class_id` int(10) NOT NULL,
  `oid_suffix_mapping_id` int(10) default NULL,
  PRIMARY KEY  (`collection_id`),
  UNIQUE KEY `ind_name` (`name`),
  UNIQUE KEY `collection_idx` (`name`),
  KEY `host_id` USING BTREE (`host_id`),
  KEY `suffix_mapping_collection_fk` (`oid_suffix_mapping_id`),
  KEY `collection_class_collection_fk` (`collection_class_id`),
  CONSTRAINT `collection_class_collection_fk` FOREIGN KEY (`collection_class_id`) REFERENCES `collection_class` (`collection_class_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `collection_ibfk_1` FOREIGN KEY (`host_id`) REFERENCES `host` (`host_id`) ON DELETE CASCADE ON UPDATE NO ACTION,
  CONSTRAINT `suffix_mapping_collection_fk` FOREIGN KEY (`oid_suffix_mapping_id`) REFERENCES `oid_suffix_mapping` (`oid_suffix_mapping_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=12917 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `collection`
--

LOCK TABLES `collection` WRITE;
/*!40000 ALTER TABLE `collection` DISABLE KEYS */;
/*!40000 ALTER TABLE `collection` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `collection_acl`
--

DROP TABLE IF EXISTS `collection_acl`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `collection_acl` (
  `acl_role_id` int(10) NOT NULL,
  `collection_id` int(8) NOT NULL,
  `can_edit` int(10) NOT NULL default '0',
  PRIMARY KEY  (`acl_role_id`,`collection_id`),
  KEY `collection_collection_acl_fk` (`collection_id`),
  CONSTRAINT `collection_collection_acl_fk` FOREIGN KEY (`collection_id`) REFERENCES `collection` (`collection_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `user_role_collection_group_user_group_acl_fk` FOREIGN KEY (`acl_role_id`) REFERENCES `acl_role` (`acl_role_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `collection_acl`
--

LOCK TABLES `collection_acl` WRITE;
/*!40000 ALTER TABLE `collection_acl` DISABLE KEYS */;
/*!40000 ALTER TABLE `collection_acl` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `collection_class`
--

DROP TABLE IF EXISTS `collection_class`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `collection_class` (
  `collection_class_id` int(10) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `description` varchar(64) default NULL,
  `collection_interval` int(8) NOT NULL,
  `default_cf` varchar(128) NOT NULL,
  `default_class` tinyint(1) default '0',
  PRIMARY KEY  (`collection_class_id`),
  UNIQUE KEY `name_idx_1` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=17 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `collection_class`
--

LOCK TABLES `collection_class` WRITE;
/*!40000 ALTER TABLE `collection_class` DISABLE KEYS */;
INSERT INTO `collection_class` VALUES (1,'JUNIPER-CPU','JUNIPER-CPU',10,'AVERAGE',0),(2,'CRS-TEMP','CRS Temperature',10,'AVERAGE',0),(3,'CRS-CPU','Cisco CPU Statistics',10,'AVERAGE',0),(4,'Interfaces','HCOctets, errors, packets',10,'AVERAGE',1),(6,'JUNIPER-TEMP','JUNIPER-TEMP',10,'AVERAGE',0),(7,'HP-CPU','HP CPU',10,'AVERAGE',0),(8,'dialup','dialup stats',30,'AVERAGE',0),(9,'vpn','VPN Statistics',30,'AVERAGE',0),(10,'HP-TEMP','HP Temperature',10,'AVERAGE',0),(11,'DC-Sentry-Power','Sentry Power DC',10,'AVERAGE',0),(12,'AC-Sentry-Power','Sentry Power AC',10,'AVERAGE',0),(13,'Interfaces-1min','HCOctets,errors,packets',60,'AVERAGE',0),(14,'CISCO-CPU','Cisco CPU statistics',10,'AVERAGE',0),(15,'CISCO-TEMP','Cisco Temp statistics',10,'AVERAGE',0),(16,'Interfaces-lowres-1min','Octets,errors,packets',60,'AVERAGE',0);
/*!40000 ALTER TABLE `collection_class` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `collection_group`
--

DROP TABLE IF EXISTS `collection_group`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `collection_group` (
  `collection_group_id` int(8) NOT NULL auto_increment,
  `name` varchar(64) default NULL,
  PRIMARY KEY  (`collection_group_id`),
  UNIQUE KEY `collection_group_idx` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `collection_group`
--

LOCK TABLES `collection_group` WRITE;
/*!40000 ALTER TABLE `collection_group` DISABLE KEYS */;
/*!40000 ALTER TABLE `collection_group` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `collection_group_user_group_acl`
--

DROP TABLE IF EXISTS `collection_group_user_group_acl`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `collection_group_user_group_acl` (
  `collection_group_id` int(8) NOT NULL,
  `user_group_id` int(10) NOT NULL,
  `can_edit` int(10) NOT NULL default '0',
  PRIMARY KEY  (`collection_group_id`,`user_group_id`),
  KEY `user_group_collection_group_user_group_acl_fk` (`user_group_id`),
  CONSTRAINT `collection_group_collection_group_user_group_acl_fk` FOREIGN KEY (`collection_group_id`) REFERENCES `collection_group` (`collection_group_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `user_group_collection_group_user_group_acl_fk` FOREIGN KEY (`user_group_id`) REFERENCES `user_group` (`user_group_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `collection_group_user_group_acl`
--

LOCK TABLES `collection_group_user_group_acl` WRITE;
/*!40000 ALTER TABLE `collection_group_user_group_acl` DISABLE KEYS */;
/*!40000 ALTER TABLE `collection_group_user_group_acl` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `collection_instantiation`
--

DROP TABLE IF EXISTS `collection_instantiation`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `collection_instantiation` (
  `collection_id` int(8) NOT NULL,
  `end_epoch` int(10) NOT NULL,
  `start_epoch` int(10) NOT NULL,
  `threshold` double default NULL,
  `description` varchar(50) NOT NULL,
  PRIMARY KEY  (`collection_id`,`end_epoch`),
  UNIQUE KEY `collection_id_startn_idx` (`collection_id`,`start_epoch`),
  CONSTRAINT `collection_collection_instantiation_fk` FOREIGN KEY (`collection_id`) REFERENCES `collection` (`collection_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `collection_instantiation`
--

LOCK TABLES `collection_instantiation` WRITE;
/*!40000 ALTER TABLE `collection_instantiation` DISABLE KEYS */;
/*!40000 ALTER TABLE `collection_instantiation` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `global`
--

DROP TABLE IF EXISTS `global`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `global` (
  `name` varchar(64) default NULL,
  `value` varchar(256) default NULL,
  KEY `k_ind` USING BTREE (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `global`
--

LOCK TABLES `global` WRITE;
/*!40000 ALTER TABLE `global` DISABLE KEYS */;
INSERT INTO `global` VALUES ('rrddir','/tmp/db');
/*!40000 ALTER TABLE `global` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `host`
--

DROP TABLE IF EXISTS `host`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `host` (
  `host_id` int(8) NOT NULL auto_increment,
  `ip_address` varchar(40) NOT NULL,
  `description` varchar(64) NOT NULL,
  `dns_name` varchar(128) default NULL,
  `community` varchar(50) NOT NULL,
  PRIMARY KEY  (`host_id`),
  UNIQUE KEY `host_idx` (`ip_address`,`community`),
  UNIQUE KEY `dns_name_ip_address_idx1` (`ip_address`,`dns_name`)
) ENGINE=InnoDB AUTO_INCREMENT=545 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `host`
--

LOCK TABLES `host` WRITE;
/*!40000 ALTER TABLE `host` DISABLE KEYS */;
/*!40000 ALTER TABLE `host` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `log`
--

DROP TABLE IF EXISTS `log`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `log` (
  `log_id` int(10) NOT NULL auto_increment,
  `ip_address` varchar(40) NOT NULL,
  `action` varchar(200) NOT NULL,
  `epoch` int(10) NOT NULL,
  `user_id` int(8) NOT NULL,
  PRIMARY KEY  (`log_id`),
  KEY `user_log_fk` (`user_id`),
  CONSTRAINT `user_log_fk` FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `log`
--

LOCK TABLES `log` WRITE;
/*!40000 ALTER TABLE `log` DISABLE KEYS */;
/*!40000 ALTER TABLE `log` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `oid_collection`
--

DROP TABLE IF EXISTS `oid_collection`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `oid_collection` (
  `oid_collection_id` int(10) NOT NULL auto_increment,
  `name` varchar(50) default NULL,
  `oid_prefix` varchar(128) NOT NULL,
  `datatype` varchar(128) NOT NULL,
  `description` varchar(64) default NULL,
  `displaytype` varchar(20) default NULL,
  `units` varchar(25) default NULL,
  `color` varchar(10) default NULL,
  `graph_math` varchar(128) default NULL,
  `default_rrd_name` varchar(20) NOT NULL,
  `default_on` tinyint(1) NOT NULL default '1',
  PRIMARY KEY  (`oid_collection_id`),
  UNIQUE KEY `oid_collection_idx1` (`oid_prefix`),
  UNIQUE KEY `oid_collection_idx` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=28 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `oid_collection`
--

LOCK TABLES `oid_collection` WRITE;
/*!40000 ALTER TABLE `oid_collection` DISABLE KEYS */;
INSERT INTO `oid_collection` VALUES (1,'JUNIPER-CPU','.1.3.6.1.4.1.2636.3.1.13.1.8','GAUGE','CPU % Utilization','LINE2','%%',NULL,NULL,'cpu',1),(2,'temp','1.3.6.1.4.1.9.9.91.1.1.1.1.4','GAUGE','RP Temperature','LINE2',' degrees C','00FFFF',NULL,'temp',1),(3,'cpu1min','1.3.6.1.4.1.9.9.109.1.1.1.1.7','GAUGE','1 minute CPU load average','LINE2','%%','00FFFF',NULL,'cpu',1),(4,'out-errors','IF-MIB::ifOutErrors','COUNTER','Outbound Errors','LINE1','eps',NULL,NULL,'outerror',0),(5,'status','IF-MIB::ifOperStatus','GAUGE','Interface Status','LINE1',NULL,NULL,NULL,'status',0),(6,'out-packets','IF-MIB::ifOutUcastPkts','COUNTER','Outbound Packets per Second','LINE2','pps','FF33FF',NULL,'outUcast',0),(7,'in-errors','IF-MIB::ifInErrors','COUNTER','Inbound Errors per Second','LINE1','eps',NULL,NULL,'inerrors',0),(8,'in-octets','IF-MIB::ifHCInOctets','COUNTER','Inbound Bits per Second','AREA','bps','00FF00','input,8,*','input',1),(9,'out-octets','IF-MIB::ifHCOutOctets','COUNTER','Outbound Bits per Second','LINE2','bps','0000FF','output,8,*','output',1),(10,'in-packets','IF-MIB::ifInUcastPkts','COUNTER','Inbound Packets per Second','AREA','pps','22FF22',NULL,'inUcast',0),(11,'JUNIPER-TEMP','.1.3.6.1.4.1.2636.3.1.13.1.7','GAUGE','RE CPU Utilization','LINE2','%%',NULL,NULL,'cpu',1),(12,'HP-cpu','.1.3.6.1.4.1.11.2.14.11.5.1.9.6.1','GAUGE','CPU % Utilization','LINE2',NULL,NULL,NULL,'cpu',1),(13,'CISCO-modems-used','1.3.6.1.4.1.9.9.47.1.1.6','GAUGE','Number of modems used','LINE1',NULL,'00FFFF',NULL,'modemsUsed',1),(14,'CISCO-memFree','.1.3.6.1.4.1.9.9.48.1.1.1.6','GAUGE','Memory Free','LINE1',NULL,'00FF00',NULL,'memFree',1),(15,'CISCO-memUsed','.1.3.6.1.4.1.9.9.48.1.1.1.5','GAUGE','Memory used','LINE1',NULL,'22FF22',NULL,'memUsed',1),(16,'CISCO-VPN-active-sessions','.1.3.6.1.4.1.3076.2.1.2.17.1.1','GAUGE','Total Active sessions','LINE1',NULL,'00ff00',NULL,'activeSess',1),(17,'CISCO-VPN-active-management','.1.3.6.1.4.1.3076.2.1.2.17.1.8','GAUGE','Current Active management sessions','AREA',NULL,'00ff00',NULL,'activeMgmt',1),(18,'CISCO-VPN-LANtoLAN','.1.3.6.1.4.1.3076.2.1.2.17.1.7','GAUGE','Active LAN to LAN Sessions','LINE2',NULL,'22ff22',NULL,'activeLAN2LAN',1),(19,'HP-switch-memtotal','.1.3.6.1.4.1.11.2.14.11.5.1.1.2.2.1.1.5','GAUGE','Memory Total',NULL,NULL,NULL,NULL,'memTotal',1),(20,'HP-switch-memfree','.1.3.6.1.4.1.11.2.14.11.5.1.1.2.2.1.1.6','GAUGE','Memory free',NULL,NULL,NULL,NULL,'memFree',1),(21,'HP-switch-memallocated','.1.3.6.1.4.1.11.2.14.11.5.1.1.2.2.1.1.7','GAUGE','Memory used',NULL,NULL,NULL,NULL,'memUsed',1),(22,'HP-temp','.1.3.6.1.2.1.99.1.1.1.4','GAUGE','HP Chassis Temperature','LINE1',NULL,'00FFFF',NULL,'temp',1),(23,'Sentry-Power-DC','.1.3.6.1.4.1.1718.3.2.3.1.7','GAUGE','AC Sentry output power',NULL,'A',NULL,NULL,'power',1),(24,'Sentry-Power-AC','.1.3.6.1.4.1.1718.3.2.2.1.7','GAUGE','DC sentry outlet power',NULL,'A',NULL,NULL,'power',1),(26,'in-octets-low','IF-MIB::ifInOctets','COUNTER','Inbound Bits per Second','LINE1','bps','00FF00','input,8,*','input',1),(27,'out-octets-low','IF-MIB::ifOutOctets','COUNTER','Outbound Bits per Second','LINE2','bps','0000FF','output,8,*','output',1);
/*!40000 ALTER TABLE `oid_collection` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `oid_collection_class_map`
--

DROP TABLE IF EXISTS `oid_collection_class_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `oid_collection_class_map` (
  `collection_class_id` int(10) NOT NULL,
  `oid_collection_id` int(10) NOT NULL,
  `order_val` int(10) NOT NULL default '20',
  `ds_name` varchar(20) NOT NULL,
  PRIMARY KEY  (`collection_class_id`,`oid_collection_id`),
  UNIQUE KEY `ds_name_collection_class_id_idx` (`collection_class_id`,`ds_name`),
  KEY `oid_collection_oid_collection_class_map_fk` (`oid_collection_id`),
  CONSTRAINT `collection_class_oid_collection_class_map_fk` FOREIGN KEY (`collection_class_id`) REFERENCES `collection_class` (`collection_class_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `oid_collection_oid_collection_class_map_fk` FOREIGN KEY (`oid_collection_id`) REFERENCES `oid_collection` (`oid_collection_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `oid_collection_class_map`
--

LOCK TABLES `oid_collection_class_map` WRITE;
/*!40000 ALTER TABLE `oid_collection_class_map` DISABLE KEYS */;
INSERT INTO `oid_collection_class_map` VALUES (1,1,20,'cpu'),(2,2,20,'temp'),(3,3,20,'cpu'),(4,4,100,'outerror'),(4,5,20,'status'),(4,6,20,'outUcast'),(4,7,100,'inerror'),(4,8,20,'input'),(4,9,20,'output'),(4,10,20,'inUcast'),(6,11,20,'temp'),(7,12,20,'cpu'),(8,1,20,'cpu'),(8,13,20,'modems_used'),(8,14,20,'memFree'),(8,15,20,'memUsed'),(9,16,20,'activeSess'),(9,17,20,'activeMgmt'),(9,18,20,'activeLAN2LAN'),(10,22,20,'temp'),(11,23,20,'power'),(12,24,20,'power'),(13,4,100,'outerror'),(13,5,20,'status'),(13,6,20,'outUcast'),(13,7,100,'inerror'),(13,8,20,'input'),(13,9,20,'output'),(13,10,20,'inUcast'),(14,3,20,'cpu'),(15,2,20,'temp'),(16,4,100,'outerror'),(16,5,20,'status'),(16,6,20,'outUcast'),(16,7,100,'inerror'),(16,10,20,'inUcast'),(16,26,20,'input'),(16,27,20,'output');
/*!40000 ALTER TABLE `oid_collection_class_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `oid_suffix_mapping`
--

DROP TABLE IF EXISTS `oid_suffix_mapping`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `oid_suffix_mapping` (
  `oid_suffix_mapping_id` int(10) NOT NULL auto_increment,
  `name` varchar(40) NOT NULL,
  `oid_suffix_mapping_value_id` int(10) NOT NULL,
  PRIMARY KEY  (`oid_suffix_mapping_id`),
  UNIQUE KEY `oid_suffix_mapping_idx` (`name`),
  UNIQUE KEY `oid_suffix_mapping_idx1` (`oid_suffix_mapping_value_id`),
  CONSTRAINT `oid_suffix_mapping_map_type_fk` FOREIGN KEY (`oid_suffix_mapping_value_id`) REFERENCES `oid_suffix_mapping_value` (`oid_suffix_mapping_value_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `oid_suffix_mapping`
--

LOCK TABLES `oid_suffix_mapping` WRITE;
/*!40000 ALTER TABLE `oid_suffix_mapping` DISABLE KEYS */;
INSERT INTO `oid_suffix_mapping` VALUES (1,'juniper-re',1),(2,'crs-temp-map',2),(3,'crs-cpu-map',4),(4,'name',5),(7,'description',9),(8,'dc_sentry_outletname',8),(9,'ac_sentry_outletname',10);
/*!40000 ALTER TABLE `oid_suffix_mapping` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `oid_suffix_mapping_value`
--

DROP TABLE IF EXISTS `oid_suffix_mapping_value`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `oid_suffix_mapping_value` (
  `oid_suffix_mapping_value_id` int(10) NOT NULL auto_increment,
  `oid` varchar(128) default NULL,
  `description` varchar(60) default NULL,
  `next_oid_suffix_mapping_value_id` int(10) default NULL,
  PRIMARY KEY  (`oid_suffix_mapping_value_id`),
  KEY `oid_suffix_mapping_value_oid_suffix_mapping_value_fk` (`next_oid_suffix_mapping_value_id`),
  CONSTRAINT `oid_suffix_mapping_value_oid_suffix_mapping_value_fk` FOREIGN KEY (`next_oid_suffix_mapping_value_id`) REFERENCES `oid_suffix_mapping_value` (`oid_suffix_mapping_value_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `oid_suffix_mapping_value`
--

LOCK TABLES `oid_suffix_mapping_value` WRITE;
/*!40000 ALTER TABLE `oid_suffix_mapping_value` DISABLE KEYS */;
INSERT INTO `oid_suffix_mapping_value` VALUES (1,'.1.3.6.1.4.1.2636.3.1.13.1.5','juniper-re',NULL),(2,'.1.3.6.1.2.1.47.1.1.1.1.7','crs-temp-map',NULL),(3,'.1.3.6.1.4.1.9.9.109.1.1.1.1.2','crs-cpu-map',NULL),(4,'.1.3.6.1.2.1.47.1.1.1.1.7','crs-cpu-map',3),(5,'.1.3.6.1.2.1.31.1.1.1.1','interface name to Instance Number',NULL),(8,'.1.3.6.1.4.1.1718.3.2.3.1.2','Sentry outletname to index_d3',NULL),(9,'.1.3.6.1.2.1.2.2.1.2','Iface description to index',NULL),(10,'.1.3.6.1.4.1.1718.3.2.2.1.2','AC Sentry outlet to index',NULL);
/*!40000 ALTER TABLE `oid_suffix_mapping_value` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `rra`
--

DROP TABLE IF EXISTS `rra`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `rra` (
  `rra_id` int(10) NOT NULL auto_increment,
  `collection_class_id` int(10) default NULL,
  `step` int(8) NOT NULL,
  `cf` varchar(128) NOT NULL,
  `num_days` int(8) NOT NULL,
  `xff` double NOT NULL,
  PRIMARY KEY  (`rra_id`),
  KEY `collection_class_id` USING BTREE (`collection_class_id`),
  CONSTRAINT `rra_ibfk_1` FOREIGN KEY (`collection_class_id`) REFERENCES `collection_class` (`collection_class_id`) ON DELETE CASCADE ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=79 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `rra`
--

LOCK TABLES `rra` WRITE;
/*!40000 ALTER TABLE `rra` DISABLE KEYS */;
INSERT INTO `rra` VALUES (37,4,1,'AVERAGE',1095,0.8),(38,4,8640,'AVERAGE',1460,0.8),(39,4,360,'AVERAGE',1460,0.8),(40,4,8640,'MAX',1460,0.8),(41,1,1,'AVERAGE',1095,0.8),(42,1,360,'AVERAGE',1460,0.8),(43,1,8640,'AVERAGE',1460,0.8),(44,2,1,'AVERAGE',1095,0.8),(45,2,360,'AVERAGE',1460,0.8),(46,2,8640,'AVERAGE',1460,0.8),(47,3,1,'AVERAGE',1095,0.8),(48,3,360,'AVERAGE',1460,0.8),(49,3,8640,'AVERAGE',1460,0.8),(50,6,1,'AVERAGE',1095,0.8),(51,6,360,'AVERAGE',1460,0.8),(52,6,8640,'AVERAGE',1460,0.8),(53,7,1,'AVERAGE',1095,0.8),(54,7,360,'AVERAGE',1460,0.8),(55,7,8640,'AVERAGE',1460,0.8),(56,10,1,'AVERAGE',1095,0.8),(57,10,360,'AVERAGE',1460,0.8),(58,10,8640,'AVERAGE',1460,0.8),(59,11,1,'AVERAGE',180,0.8),(60,11,60,'AVERAGE',1095,0.8),(61,12,1,'AVERAGE',180,0.8),(62,12,60,'AVERAGE',1095,0.8),(63,13,1,'AVERAGE',1095,0.8),(64,13,60,'AVERAGE',1460,0.8),(65,13,1440,'AVERAGE',1460,0.8),(66,13,60,'MAX',1460,0.8),(67,14,1,'AVERAGE',1095,0.8),(68,14,360,'AVERAGE',1460,0.8),(69,14,8640,'AVERAGE',1460,0.8),(70,15,8640,'AVERAGE',1460,0.8),(71,15,360,'AVERAGE',1460,0.8),(72,15,1,'AVERAGE',1095,0.8),(73,4,30,'AVERAGE',1460,0.8),(74,13,5,'AVERAGE',1460,0.8),(75,16,1,'AVERAGE',1095,0.8),(76,16,60,'AVERAGE',1460,0.8),(77,16,1440,'AVERAGE',1460,0.8),(78,16,60,'MAX',1460,0.8);
/*!40000 ALTER TABLE `rra` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user`
--

DROP TABLE IF EXISTS `user`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `user` (
  `user_id` int(8) NOT NULL auto_increment,
  `name` varchar(30) NOT NULL,
  `comment` varchar(64) NOT NULL,
  `email` varchar(64) NOT NULL,
  `active` int(10) NOT NULL default '1',
  PRIMARY KEY  (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `user`
--

LOCK TABLES `user` WRITE;
/*!40000 ALTER TABLE `user` DISABLE KEYS */;
INSERT INTO `user` VALUES (1,'snapp-gen','SNAPP Config Gen','syseng@grnoc.iu.edu',1);
/*!40000 ALTER TABLE `user` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user_group`
--

DROP TABLE IF EXISTS `user_group`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `user_group` (
  `user_group_id` int(10) NOT NULL auto_increment,
  `name` varchar(50) NOT NULL,
  PRIMARY KEY  (`user_group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `user_group`
--

LOCK TABLES `user_group` WRITE;
/*!40000 ALTER TABLE `user_group` DISABLE KEYS */;
/*!40000 ALTER TABLE `user_group` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user_role_membership`
--

DROP TABLE IF EXISTS `user_role_membership`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `user_role_membership` (
  `user_id` int(8) NOT NULL,
  `acl_role_id` int(10) NOT NULL,
  PRIMARY KEY  (`user_id`,`acl_role_id`),
  KEY `acl_role_user_role_membership_fk` (`acl_role_id`),
  CONSTRAINT `acl_role_user_role_membership_fk` FOREIGN KEY (`acl_role_id`) REFERENCES `acl_role` (`acl_role_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `user_user_role_membership_fk` FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `user_role_membership`
--

LOCK TABLES `user_role_membership` WRITE;
/*!40000 ALTER TABLE `user_role_membership` DISABLE KEYS */;
INSERT INTO `user_role_membership` VALUES (1,0);
/*!40000 ALTER TABLE `user_role_membership` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `view_groups`
--

DROP TABLE IF EXISTS `view_groups`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `view_groups` (
  `views_id` int(8) NOT NULL,
  `category_id` int(10) NOT NULL,
  PRIMARY KEY  (`views_id`,`category_id`),
  KEY `category_view_groups_fk` (`category_id`),
  CONSTRAINT `category_view_groups_fk` FOREIGN KEY (`category_id`) REFERENCES `category` (`category_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `views_view_groups_fk` FOREIGN KEY (`views_id`) REFERENCES `views` (`views_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `view_groups`
--

LOCK TABLES `view_groups` WRITE;
/*!40000 ALTER TABLE `view_groups` DISABLE KEYS */;
/*!40000 ALTER TABLE `view_groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `view_user_group_acl`
--

DROP TABLE IF EXISTS `view_user_group_acl`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `view_user_group_acl` (
  `views_id` int(8) NOT NULL,
  `user_group_id` int(10) NOT NULL,
  PRIMARY KEY  (`views_id`,`user_group_id`),
  KEY `user_group_view_user_group_acl_fk` (`user_group_id`),
  CONSTRAINT `user_group_view_user_group_acl_fk` FOREIGN KEY (`user_group_id`) REFERENCES `user_group` (`user_group_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `views_view_user_group_acl_fk` FOREIGN KEY (`views_id`) REFERENCES `views` (`views_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `view_user_group_acl`
--

LOCK TABLES `view_user_group_acl` WRITE;
/*!40000 ALTER TABLE `view_user_group_acl` DISABLE KEYS */;
/*!40000 ALTER TABLE `view_user_group_acl` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `views`
--

DROP TABLE IF EXISTS `views`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `views` (
  `views_id` int(8) NOT NULL auto_increment,
  `name` varchar(64) default NULL,
  `rra_cf` varchar(128) default NULL,
  `filter_regexp` varchar(128) default NULL,
  `custom` tinyint(1) NOT NULL,
  PRIMARY KEY  (`views_id`),
  UNIQUE KEY `views_idx` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `views`
--

LOCK TABLES `views` WRITE;
/*!40000 ALTER TABLE `views` DISABLE KEYS */;
/*!40000 ALTER TABLE `views` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `views_data`
--

DROP TABLE IF EXISTS `views_data`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `views_data` (
  `views_id` int(8) NOT NULL auto_increment,
  `ds_name` varchar(20) NOT NULL,
  PRIMARY KEY  (`views_id`),
  CONSTRAINT `views_data_ibfk_1` FOREIGN KEY (`views_id`) REFERENCES `views` (`views_id`) ON DELETE CASCADE ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `views_data`
--

LOCK TABLES `views_data` WRITE;
/*!40000 ALTER TABLE `views_data` DISABLE KEYS */;
/*!40000 ALTER TABLE `views_data` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2010-10-05  0:40:11
