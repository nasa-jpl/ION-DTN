#!/usr/bin/env perl
package ION::Test;
our $VERSION = 0.0;

use v5.10;
use strict;
use warnings;
use Cwd;
use Term::ANSIColor qw(:constants);
use Expect; # TODO: Define this in a way that script can run even if not installed
local $Term::ANSIColor::AUTORESET = 1;
use File::Slurp;
use File::Spec;
use Data::Dumper; # For debug purposes

our @ISA = qw(Exporter);
our @EXPORT = qw(create_test_file spawn_program kill_program);

=head1 ION Test Driver Library

This module contains common functions to drive automated tests of ION.

=cut

sub new
{
    my $class = shift;

    my %default = (
        # Settings
        verbose => 0,
        test_prompt => 0,
        time_display_text => 2,
        time_wait_finish => 5,
        exit_on_fail => 0,
        spawn_mode => "term", # term or expect
        spawn_term => "xterm", # Terminal program to launch if mode is term
        skip_initial_cleanup => 0, # If 1, skip cleanup during initial startup (assume clean slate)
        
        # State/Status
        tests_run => 0,
        tests_failed => 0,
        tests_skipped => 0,
        tests_passed => 0,
        tests_configured => 0,
        summary_log => "",
        fileno => {},

        spawned => [], # For logging/cleanup of spawned commands (TODO)
        spawn_stopped => [],
        );
    my %obj = (%default, @_);

    $obj{log_dir} = File::Spec->rel2abs($obj{log_dir}) if $obj{log_dir};
    
    if (defined($obj{log_file})) {
        $obj{log_file} = File::Spec->catfile($obj{log_dir}, $obj{log_file}) if $obj{log_dir};

        say CYAN "log_file is: ".$obj{log_file};
        rename($obj{log_file}, $obj{log_file}.".bak") if -f $obj{log_file};
        my $time = localtime();
        write_file($obj{log_file}, "Script started at $time");
    }
    
    return bless \%obj;
}
sub log
{
    my $self = shift;
    my $msg = shift;
    my $fn = $self->{log_file};

    say YELLOW $msg if $self->{verbose};
    
    $msg .= "\n" unless $msg =~ /\n$/;

    # say $fd $msg if $fd; # Deprecated
    append_file($fn, $msg) if $fn;
}

sub global_startup
{
    my $self = shift;
    
    # Set Enviornment Variable for Multi-node tests
    $ENV{'ION_NODE_LIST_DIR'} = getcwd();
    
    # Cleanup any prior tests
    $self->cleanup() unless $self->{skip_initial_cleanup};

    # Initial Startup of ION Nodes
    $self->start_node(2);
    $self->start_node(3);
    $self->start_node(4);

    $self->log( "Started nodes 2,3,4" );
}

sub global_shutdown
{
    my $self = shift;
    
    $self->stop_node(2);
    $self->stop_node(3);
    $self->stop_node(4);
    sleep(2);
    $self->stop_all();
    say YELLOW "Waiting for nodes to shutdown";
    sleep(5); # Give nodes time to shutdown
    system("killm"); # And cleanup anything that failed to exit gracefully

    $self->log( "ION Shutdown");
}

sub show_results
{
    my $self = shift;
    
    # Show results
    say BOLD "Test sequence completed. $self->{tests_configured} test configurations defined.";
    say $self->{summary_log};
    if ($self->{tests_failed} > 0) {
        say RED "$self->{tests_failed} test(s) failed of $self->{tests_run} executed.";
    } else {
        say GREEN "$self->{tests_run} test(s) passed. No failures detected.";
    }
    say YELLOW "$self->{tests_skipped} test(s) skipped." if $self->{tests_skipped} > 0;

    return $self->{tests_failed};
}

sub cleanup
{
    my $self = shift;
    say BOLD "Cleaning up prior tests . . . ";
    unlink glob "*/ion.log";
    unlink glob "*/*_results.txt";
    unlink glob "*/*_receive_file.txt";
    unlink 'ion_nodes';
    system('killm');
    $self->log( "ION Log Cleanup Complete" );
}


sub start_node
{
    my $self = shift;
    my $node = shift;
    say BOLD "Starting node $node";

    chdir "$node.ipn.ltp" || die "Can't enter config dir for node $node: $!";
    system("ionadmin amroc.ionrc");
    system("ionadmin global.ionrc");
    system("ionsecadmin amroc.ionsecrc");
    system("ltpadmin amroc.ltprc");
    system("bpadmin amroc.bprc");
    chdir "..";
}
sub stop_node
{
    my $self = shift;
    my $node = shift;
    say BOLD "Stopping node $node";
    chdir("$node.ipn.ltp");
    system("./ionstop &");
    chdir("..");
}

sub run_tests
{
    my $self = shift;
    my $tests = shift;
    my $idx = shift;
    
    $self->{tests_configured} = scalar(@$tests);

    if (defined($idx)) {
        if ($idx >= 0 && $idx < @$tests) {
            say CYAN "Running singular Test $idx";
            $self->run_test($idx, @$tests[$idx]);
        } else {
            say RED "$idx is not a valid test number";
        }
    } else {
    
        for (my $i = 0; $i<@$tests; $i++) {
            # Call Referenced function, passing in any arguments defined in test case
            my $rtv = $self->run_test($i, @$tests[$i]);
            
            last if ($rtv < 0 && $self->{exit_on_fail});
            $self->test_prompt($tests) if $rtv < 0 && $self->{prompt_on_fail};
        }
        say CYAN "All tests completed";
    }

    # Interactive Prompt
    if ($self->{test_prompt}) {
        return $self->test_prompt($tests);
    } elsif ($self->{prompt_on_fail} && $self->{tests_failed} > 0)  {
        $self->show_results();
        return $self->test_prompt($tests);
    }
}

sub test_prompt
{
    my $self = shift;
    my $tests = shift;
    
    say "Press 'r' to restart sequence, 'q' to quit, a number of a specific test to rerun, 'l' to list, or any other key to proceed";
    my $tmp = <STDIN>;
    chomp($tmp);
    if ($tmp eq "r") {
        return $self->run_tests($tests);
    } elsif ($tmp eq "q") {
        die("User aborted test sequence");
    } elsif ($tmp eq "l") {
        return $self->list_tests($tests);
    } elsif ($tmp =~ /^\d+$/) { # If input was numeric
        return $self->run_tests($tests, $tmp);
    }

    return $tmp;
}

sub list_tests
{
    my $self = shift;
    my $tests = shift;

    say "Available Test Cases are";
    for (my $i = 0; $i < @$tests; $i++) {
        my $tst = @$tests[$i];
        say "$i: @$tst[0]";
        if (defined(@$tst[2])) {
            say CYAN "\t Description: @$tst[2]->{description}" if defined(@$tst[2]->{description});
            say YELLOW "\t Comment: @$tst[2]->{comment}" if defined(@$tst[2]->{comment});
            say RED "\t Disabled: @$tst[2]->{disabled}" if defined(@$tst[2]->{disabled});
        }
    }

    if ($self->{test_prompt}) {
        return $self->test_prompt($tests);
    }
}

sub run_test
{
    my $self = shift;
    my $idx = shift;
    my $test = shift;

    say BOLD "Running test $idx: @$test[0]";
    $self->log("Running test $idx: @$test[0]");
    
    if ($self->{test_prompt}) {
        say "Press any key to proceed";
        my $tmp = <STDIN>;
    } elsif ($self->{time_display_text} > 0) {
        say YELLOW "Waiting $self->{time_display_text}s before proceeding";
        sleep($self->{time_display_text});
    }

    # Check if subtest is disabled
    if (defined(@$test[2]) && defined(@$test[2]->{disabled})) {
        say BOLD YELLOW "Skipped disabled test: @$test[2]->{disabled}";
        $self->{tests_skipped}++;
        return 0;
    }

    # Call pre-test hook, if defined
    if ($self->{beforeEach}) {
        $self->{beforeEach}($idx, @$test[2]);
    }
    
    $self->{tests_run}++;
    
    my $result = &{@$test[1]}($idx, @$test[2]);
    if ($result == 1) {
        $self->{tests_passed}++;
        say BOLD GREEN "Test Successful";
    } elsif ($result < 0) {
        say BOLD RED "Test Failed";
        $self->{summary_log} .= "$idx: Failed\n";
        $self->{tests_failed}++;
    } elsif ($result == 0) {
        say BOLD YELLOW "Test Skipped";
        $self->{tests_skipped}++;
    } else {
        say RED "Warning: Test returned unrecognized status of $result";
        $self->{summary_log} .= "$idx: Returned unrecognized status of $result\n";
    }
    $self->log("Test $idx complete with status $result");
    say CYAN "Test Completed";
    return $result;
}

sub run_admin_cmd
{
    my $self = shift;
    my $node = shift; # Used for verbose output only
    my $app = shift;
    my $cmd = shift;

    print CYAN "($node) $app << $cmd ";
    
    my $result = `$app <<EOF
$cmd
EOF`;
    say $result if $self->{verbose};

    if ($result =~ /Command failed/) {
        say RED "FAILED";
        return 0;
    } else {
        say GREEN "SUCCESS";
        return 1;
    }        
}

# Standard Test function using bpsource/bpecho/bptrace/bpsendfile/bprecvfile as appropriate, supporting various options
#   If not specified, Node 2 is Tx and Node4 is Rx, with Hop not configured.
# TODO: Consider adding function references to opts for optional added setup, ie: configuring security profiles for appropriate tests.  This function can then move into common test library
sub full_test {
    my $self = shift;
    my $idx = shift; # Test Number
    my $opts = shift;
    my $rtv = 0;

    my $txNode = $opts->{'txNode'} || 2;    
    my $rxNode = $opts->{'rxNode'} || 3;
    my $hopNode = $opts->{'hopNode'};

    # Setup Nodes
    # If expectSetupFailure, we invert return value on setup failure
    my $setupRtv = (defined($opts->{'expectSetupFailure'})) ? 1 : -1;

    if (defined($self->{base_test_cleanup})) {
        $self->base_test_cleanup($rxNode);
        $self->base_test_cleanup($txNode);
        $self->base_test_cleanup($hopNode) if $hopNode;
    }
    
    if (defined($self->{base_test_setup})) {
        $self->base_test_setup($rxNode) || return $setupRtv;
        $self->base_test_setup($txNode) || return $setupRtv;
        
        if ($hopNode) {
            $self->base_test_setup($hopNode) || return $setupRtv;
        }

    }

    if (defined($opts->{'expectSetupFailure'})) {
        say RED "Test Setup unexpectedly succeeded";
        return -1;
    }
    
    

    # Generate Data to Transmit
    if (defined($opts->{'size'})) {
        # File based Transmission

        # Send the file
        chdir("$txNode.ipn.ltp");
        create_test_file($opts->{'size'});
        system("bpsendfile ipn:$txNode.1 ipn:$rxNode.2 hugefile.big");
        chdir("..");

        say YELLOW "Waiting $self->{time_wait_finish}s";
        sleep($self->{time_wait_finish});

        # Verify
        my $fileno = $self->{fileno}{$rxNode} || 1;
        my $fn = "$rxNode.ipn.ltp/testfile$fileno";
        if (-e $fn) {
            # Ensure future tests look at the correct file
            $self->{fileno}{$rxNode} = $fileno+1;

            # And compare the results
            if (compare($fn, "$txNode.ipn.ltp/hugefile.big") != 0) {
                say RED "File received but NOT equal";
                $rtv = -1;

                # Note: We do not delete the file on failure to facilitate manual inspection
            } else {
                say GREEN "File received";
                $rtv = 1;
                
                # Unlink file to avoid confusion on future tests
                unlink($fn);
            }
            
        } else {
            say RED("File NOT received"), " ($fn)";
            $rtv = -1;
        }
    } else { # String-based Transmission
        my $str = "test${idx}_trace";

        chdir("$txNode.ipn.ltp");
        if ($opts->{'sendAnon'}) {
            say CYAN "($txNode) bpsource ipn:$rxNode.1 $str";
            system("bpsource ipn:$rxNode.1 \"$str\"");
        } else {
            my $cmd = "bptrace ipn:$txNode.1 ipn:$rxNode.1 ipn:$txNode.0 3600 1.1 \"$str\" rcv,ct,fwd,dlv,del";
            say CYAN "($txNode) $cmd";
            system($cmd);
        }
        chdir("..");
        say YELLOW "Waiting $self->{time_wait_finish}s";
        sleep($self->{time_wait_finish});
        
        # Verify results & return
        my $grep = `grep "$str" $rxNode.ipn.ltp/${rxNode}_results.txt`;
        if ($? != 0) {
            $rtv = -1;
            say RED "Payload NOT received";
        } else {
            say GREEN "Payload received";
            $rtv = 1;
        }
    }
    
    $rtv = -$rtv if $opts->{'expectFailure'};

    if (defined($self->{base_test_cleanup})) {
        $self->base_test_cleanup($txNode);
        $self->base_test_cleanup($hopNode) if (defined($hopNode));
        $self->base_test_cleanup($rxNode);
    }

    
    if ($rtv < 0) {
        say RED "Test $idx failed.";
        return -1;
    } else {
        return 1;
    }

}


## Non-Object Functions
sub create_test_file
{
    my $size = shift;
    my $name = shift || "hugefile.big";
    
    system("head -c $size < /dev/urandom > $name") == 0 or die "File Generation failed: $?";
}    

sub spawn_program
{
    my $cmd = shift;
    my $pid = fork();
    die "Unable to fork: $!" unless defined($pid);
    if (!$pid) {
        # Child process
        # TODO/FIXME: This does not suppress stdout/stdin
        # TODO: Look at IPC::Run instead
        exec($cmd);
        die "Unable to exec: $!";
    } else {
        # Parent Process
        return $pid;
    }    
}

sub kill_program
{
    my $pid = shift;
    my $verbose = shift;
    say YELLOW "Sending SIGTERM to $pid" if $verbose;
    return kill 'SIGTERM', $pid;
}

sub start_cmd
{
    my $self = shift;
    my $mode = $self->{spawn_mode};

    if ($mode eq "term") {
        return $self->start_term(@_);
    } elsif ($mode eq "expect") {
        return $self->start_expect(@_);
    } elsif ($mode eq "screen") {
        return $self->start_screen(@_);
    } else {
        die "CONFIGURATION ERROR, Invalid mode: $mode";
    }
}
sub stop_cmd
{
    my $self = shift;
    my $mode = $self->{spawn_mode};

    if ($mode eq "term") {
        return $self->stop_term(@_);
    } elsif ($mode eq "expect") {
        return $self->stop_expect(@_);
    } elsif ($mode eq "screen") {
        return $self->stop_screen(@_);
    } else {
        die "CONFIGURATION ERROR, Invalid mode: $mode";
    }
}
sub stop_all
{
    my $self = shift;
    my $mode = $self->{spawn_mode};

    if ($mode eq "screen") {
        $self->kill_screens();
    } # Not currently implemented for other modes
}

sub start_screen
{
    my $self = shift;
    
    my $opts;
    $opts = shift if (ref($_[0]) ne ""); # Options struct is optional first argument
    my $in_cmd = shift; 

    my $title = $opts->{title} // "dotest-cmd";
    my $cwd = getcwd();
    my $screen_name = $opts->{screen_name} // "dotest";
    my $cmd;

    $in_cmd = join(' ', $in_cmd, @_) if @_;
    # TODO: Escape any double-quotes in in_cmd if needed
    
    if (!$self->{screen_initialized} ) {
        # For first start, we need to specify the log file Note:
        # Specifying a file without %n wildcard will consolidate all windows into a single log file
        #my $log_file = $opts->{log_file} // "screen_%n.log"; # One Log file with combined output from all screens?
        #$log_file = File::Spec->catfile($self->{log_dir}, $log_file) if $self->{log_dir};
        #rename($log_file, $log_file.".bak") if -f $log_file;

        # Start screen session with blank terminal in background
        $cmd = "screen -d -m -S \"$screen_name\" -t \"$title\" -L $in_cmd"; #-Logfile $log_file $in_cmd";       
        
        $self->log("Starting new screen session with title $title");
        $self->{screen_initialized} = 1;
    } else {
        say CYAN "Spawning: $in_cmd" if $self->{verbose};
        # Ensure command is executed from the correct directory
        write_file("tmp.sh", "cd $cwd && $in_cmd");
        
        $cmd = "screen -S \"$screen_name\" -t \"$title\" -L -X screen bash $cwd/tmp.sh";
    }
    say YELLOW "Spawning $cmd";
    say `$cmd`;
    die "Error spawning $cmd: $!" if $!;

    $self->{screen_initialized}++;
    
    
    $self->log("Spawned $cmd");
    
    return $in_cmd;

}
sub stop_screen
{
    my $self = shift;
    my $in_cmd = shift;

    # Get PID of spawned command
    my $pids = `ps -A -o pid,command`;
    my ($pid) = $pids =~ /^\s*(\d+)\s+$in_cmd$/m;
    if (!defined($pid)) {
        say RED "Can't determine PID for $in_cmd from, unable to stop it.";
        return -1;
    }

    # Send SIGTERM signal
    kill('SIGTERM',$pid);
    
    $self->{screen_stopped}++;
}
sub kill_screens
{
    my $self = shift;
    my $screen_name = $self->{screen_name} // "dotest";
    my $cmd = "screen -S \"$screen_name\" -X quit";
    say YELLOW "Killing screen: $cmd" if $self->{verbose};
    say `$cmd`;
}

=head2 start_term

Spawn application using configured terminal.  The following terminals have been tested
- xterm
- terminology - Does not support file logging

=cut

sub start_term
{
    my $self = shift;
    my $opts;
    $opts = shift if (ref($_[0]) ne ""); # Options struct is optional first argument
    my $in_cmd = shift; 

    my $cnt = scalar(@{$self->{spawned}});
       
    my $title = $opts->{title} // "$cnt-$in_cmd";
    my $log_file = $opts->{log_file} // "term_".$cnt."_$in_cmd.log";
    rename($log_file, $log_file.".bak") if -f $log_file;
    
    my $term = $self->{spawn_term};
    my $term_args = "";
    $term_args .= "-l -lf $log_file" if $term eq "xterm";

    
    $in_cmd = join(' ', $in_cmd, @_) if @_;
    # TODO: Escape any double-quotes in in_cmd if needed
    
    # Launch $term with $title to run $cmd and log output to $logfile
    my $cmd = "$term $term_args -T \"$title\" -e \"$in_cmd\"";
    say CYAN "Spawning: $cmd" if $self->{verbose};
    
    my $pid = spawn_program($cmd, @_);
    push(@{$self->{spawned}}, $pid );
    return $pid;
}
sub stop_term
{
    my $self = shift;
    my $pid = shift;

    return kill_program($pid, $self->{verbose});
}

my $exp_cnt = 0; # TODO: Replace with scalar(@$self->{spawned})
sub start_expect
{
    my $self = shift;
    my $opts;
    $opts = shift if (ref($_[0]) ne ""); # Options struct is optional first argument
    my $cmd = shift;
    my $exp = new Expect;
    $exp->raw_pty(1);
    $exp->log_stdout(1); # DEBUG: Trying to get logging to work

    if ($opts && defined($opts->{log_file})) {
        $exp->log_file($opts->{log_file}, "w");
    } elsif ($opts && defined($opts->{log_fn})) { # Logging function
        $exp->log_file($opts->{log_fn});
    } else {
        $exp->log_file("expect_".$exp_cnt."_$cmd.log", "w");
    }
    $exp->print_log_file("DEBUG: About to spawn\n");

    say "Spawning $cmd via Expect";
    $exp->spawn($cmd, @_) || die "Cannot spawn $cmd: $!";
    #$exp->send("\n"); # DEBUG
    $exp->print_log_file("DEBUG: Spawn complete\n");
    $exp_cnt++;
    push(@{$self->{spawned}}, $exp);
    return $exp;
}
sub stop_expect
{
    my $self = shift;
    my $exp = shift;
    $exp->soft_close();

    # Update accounting
    push(@{$self->{spawn_stopped}}, $exp);
    # TODO: Remove from self->spawned
}
sub do_expect
{
    my $self = shift;
    my $exp = shift;

    # TODO: Do we need a wrapper here?
}
sub do_expect_cmd # TODO/PROTOTYPE
{
    my $self = shift;
    my $exp = shift;
    my $cmd = shift;
    my $opts = shift;
    $exp->clear_accum(); # Reset any pending data

    # Optional setup command + expect validation

    $cmd .= "\n" unless $cmd =~ /\n$/;
    $exp->send($cmd);

    $exp->expect( ( defined($opts->{timeout}) ) ? $opts->{timeout} : 1,
                   $opts->{pattern}
                  );
                  # TODO: Handle timeout
}


1;
