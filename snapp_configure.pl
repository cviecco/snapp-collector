#!/usr/bin/perl -w
# sascii - Show ASCII values for keypresses

use strict;

use Term::ReadKey;
use XML::Writer;
use IO::File;
use DBI;


my $db_port=3306;
my $db_host='localhost';
my $snapp_db_name="snapp_test";
my $snapp_db_user="snapp";
my $snapp_db_passwd="secret";
my $snapp_control_port="9967";
my $snapp_control_passwd="control-caos";

my $snapp_data_base_dir="/tmp/snapp/db";
my $snapp_config_filename="/tmp/snapp_config.xml";

sub get_string{
   my $regexp_in=shift;
   my $default_value=shift;
   
   my $return_value=$default_value;   

   my $temp=ReadLine 0;
   chomp $temp;
   if( $temp =~/$regexp_in/){
      $return_value=$temp;
   }
   else{
      if(length($temp)!=0){
         print "  invalid entry, using default=$default_value\n";
      }
   }
   return $return_value;
}

sub create_database{

   print "Do you need to create the database? (requires 'root' db access) ? [Yn]";
   my $need_create_db=get_string("^[Yn]\$","Y");
   if($need_create_db eq 'n'){
      return 0;
   }


   print "Enter your database 'root' username: ";
   my $db_root_user=get_string("[-.a-zA-Z0-9]\$",'root');
   print "Enter your database 'root' password ";
   ReadMode 'noecho';
   my $db_root_passwd=get_string("[-.a-zA-Z0-9]\$",'password');
   ReadMode 'normal';   
   print "\n";


   #1. create db
   #2. put tables
   #3. grant privileges and flush
   my $dbh = DBI->connect("DBI:mysql:mysql;host=$db_host;port=$db_port",
                        $db_root_user, $db_root_passwd
	           ) or die "Could not connect to the db";
   my $sth = $dbh->prepare("create database if not exists $snapp_db_name") or die "cannot prepare\n";
   $sth->execute() or die "cannot execute db create";
   
   my $db_create_command="mysql -u $db_root_user -p$db_root_passwd $snapp_db_name< sql/base_example.sql";
   my $db_out=`$db_create_command`;
   
   my $sth2= $dbh->prepare("grant all privileges ON $snapp_db_name".".* to '$snapp_db_user'\@'localhost' identified by '$snapp_db_passwd'") or die "cannot prepare\n";
   $sth2->execute() or die "cannot execute grant grantprivs\n";

   $sth = $dbh->prepare("flush privileges") or die "cannot prepare\n";
   $sth->execute() or die "cannot execute db create";




   $dbh->disconnect();
   

    print "Database created and initialized\n";

}

sub write_snapp_config{
   my $config_filename=shift;

  my $output = new IO::File(">$config_filename");
  
  die "Failed to open output" if not defined $output;

  my $writer = new XML::Writer(OUTPUT => $output);
  $writer->startTag("snapp-config");
  $writer->startTag('name');
  $writer->characters("SNAPP");
  $writer->endTag('name');
  $writer->startTag('db',
                    "type" => "mysql",
                    "name" => $snapp_db_name,
                    "username" => $snapp_db_user, 
                    "password" => $snapp_db_passwd,
                    "port" => $db_port,
                    "host" => $db_host,
                    );
  $writer->endTag('db');
  $writer->startTag('control',
                    "port" => $snapp_control_port, 
                    "enable_password"=> $snapp_control_passwd);
  $writer->endTag('control');

  $writer->endTag("snapp-config");
  $writer->end();
  $output->close(); 

  print "configuration file created as '$config_filename'\n";    

}

sub update_db_config{
   my $dbh = DBI->connect("DBI:mysql:$snapp_db_name;host=$db_host;port=$db_port",
                        $snapp_db_user, $snapp_db_passwd
                   ) or die "Could not connect to the db";  

   my $sth = $dbh->prepare("delete from global where name='rrddir'") or die "cannot prepare\n";
   $sth->execute() or die "cannot execute db create";

   $sth = $dbh->prepare("insert into global (name,value) VALUES ('rrddir','$snapp_data_base_dir')") or die "cannot prepare\n";
   $sth->execute() or die "cannot execute db create";




   $dbh->disconnect();

}

sub get_base_settings{

   #print "Please enter mysql host [$db_host]";
   #$db_host=get_string("^[-.a-zA-Z0-9]+\$",$db_host);
   #print "db_host='$db_host'\n"; 

   print "Please enter mysql port [$db_port]";
   $db_port=get_string("^[0-9]+\$",$db_port);

   print "Please enter the snapp db name [$snapp_db_name]";
   $snapp_db_name=get_string("^[-_a-zA-Z0-9]+\$",$snapp_db_name);

   print "Please enter the snapp db username [$snapp_db_user]";
   $snapp_db_user=get_string("^[-_a-zA-Z0-9]+\$",$snapp_db_user);


   print "Please enter the snapp db password [$snapp_db_passwd]";
   $snapp_db_passwd=get_string("[-.a-zA-Z0-9]",$snapp_db_passwd);

   print "Please enter the snapp control_port [$snapp_control_port]";
   $snapp_control_port=get_string("^[0-9]+\$",$snapp_control_port);


   print "Please enter the snapp control password [$snapp_control_passwd]";
   $snapp_control_passwd=get_string("[-.a-zA-Z0-9]",$snapp_control_passwd);

   print "Please enter the snapp base rrd dir [$snapp_data_base_dir]";
   $snapp_data_base_dir=get_string("[-.a-zA-Z0-9]",$snapp_data_base_dir);

   print "Please enter the config filename [$snapp_config_filename]";
   $snapp_config_filename=get_string("[-.a-zA-Z0-9]",$snapp_config_filename);
   



};


sub welcome{
   print "This is the snapp configuratin interviewer, it will make sure you have\n";
   print "A base valid running snapp instance. Press enter if you want to use the\n";
   print "default values. This script assumes a local mysql database.\n";
}

sub done_success{
   print "\n";
   print "The snapp configuration is now ready, you can now start snapp by\n";
   print "running 'snapp-collector -c $snapp_config_filename' would start the collector\n"
   #print "\n";

}
sub main{
   welcome();
   get_base_settings();
   create_database();
   update_db_config();
   write_snapp_config($snapp_config_filename);
 
   done_success();  
   
   
}

main();
