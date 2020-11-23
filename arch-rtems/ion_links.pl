#!/usr/bin/env perl

=head1 SYNOPSIS

This script can create, update, or remove symlinks of ION source files to aide manual builds for embedded targets.

Include files are not symlinked and should be handled by providing the appropriate include (-I) flags to your compiler. 

If run without arguments, it will create the default set of C symlinks to the current directory ('.') assuming that the root of the ION repository is in the parent directory ('..').  To overwrite these values explicitly:

 ./ion_links.pl --src .. --tgt .


=cut

use strict;
use warnings;
use v5.10;
use Getopt::Long;
use Cwd qw(getcwd);
use File::Spec;
use Data::Dumper; # debug
my $base = "..";
my $tgt = ".";
my ($help, $man);
my $verbose = 1;
my $use_bpv7 = 1;
my $link_all = 0;
my $link_nm = 1;
my $link_ion = 1;

Getopt::Long::GetOptions(
    "help"            => \$help,
    "man"             => \$man,
    "verbose!"  => \$verbose,
    "src=s" => \$base,
    "tgt=s" => \$tgt,
    "bpv7!" => \$use_bpv7,
    "nm!" => \$link_nm,
    "ion!" => \$link_ion,
                        );
pod2usage(1) if $help;
pod2usage(-exitval => 0, -verbose => 2) if $man;



my @ici_sources = qw(
			ici/library/cbor.c 
			ici/library/crc.c 
			ici/library/ion.c 
			ici/library/ionsec.c 
			ici/library/libicinm.c 
			ici/library/llcv.c 
			ici/library/lyst.c 
			ici/library/memmgr.c 
			ici/library/platform.c 
			ici/library/platform_sm.c 
			ici/library/psm.c 
			ici/library/rfx.c 
			ici/library/smlist.c 
			ici/library/smrbt.c 
			ici/library/sptrace.c 
			ici/library/zco.c 
			ici/sdr/sdrcatlg.c 
			ici/sdr/sdrhash.c 
			ici/sdr/sdrlist.c 
			ici/sdr/sdrmgt.c 
			ici/sdr/sdrstring.c 
			ici/sdr/sdrtable.c 
			ici/sdr/sdrxn.c 
			ici/bulk/STUB_BULK/bulk.c
    );
my @ici_utils = qw(
                      ici/utils/sdrwatch.c
ici/utils/sdrmend.c
ici/utils/psmwatch.c
ici/utils/ionadmin.c
ici/utils/ionsecadmin.c
ici/utils/ionwarn.c
ici/utils/ionunlock.c
ici/utils/ionlog.c
                 );
my @ltp_sources = qw(
	ltp/library/libltp.c 
	ltp/library/libltpnm.c 
	ltp/library/libltpP.c 
	ltp/library/ltpei.c 
	ltp/library/ltpsec.c 
	ltp/sda/libsda.c
ltp/udp/libudplsa.c
ltp/utils/ltpadmin.c
ltp/utils/ltpsecadmin.c
ltp/test/ltpdriver.c
ltp/test/ltpcounter.c
ltp/test/sdatest.c
ltp/daemon/ltpclock.c
ltp/daemon/ltpmeter.c
ltp/daemon/ltpdeliv.c
ltp/udp/udplsi.c
ltp/dccp/dccplsi.c
    );
# ltp/udp/udplso.c  # TODO: needs updates for VxWorks
my @dgr_sources = qw(
    dgr/library/libdgr.c
dgr/test/file2dgr.c
dgr/test/file2udp.c
dgr/test/udp2file.c
dgr/test/file2tcp.c
dgr/test/tcp2file.c
    );

my @bp_sources; # Quick approach
if ($use_bpv7) {
@bp_sources = qw(
	bpv7/bibe/bibe.c 
	bpv7/dtn2/libdtn2fw.c 
	bpv7/library/libbp.c 
	bpv7/library/libbpP.c 
	bpv7/library/libbpnm.c 
	bpv7/library/eureka.c 
	bpv7/library/bei.c 
	bpv7/library/bpsec.c 
	bpv7/library/ext/bae/bae.c 
	bpv7/library/ext/bpq/bpq.c 
	bpv7/library/ext/bpsec/bcb.c 
	bpv7/library/ext/bpsec/bib.c 
	bpv7/library/ext/bpsec/bpsec_instr.c 
	bpv7/library/ext/bpsec/bpsec_util.c 
	bpv7/library/ext/bpsec/profiles.c 
	bpv7/library/ext/hcb/hcb.c 
	bpv7/library/ext/meb/meb.c 
	bpv7/library/ext/pnb/pnb.c 
	bpv7/library/ext/pnb/pnb.h 
	bpv7/library/ext/snw/snw.c 
	bpv7/ipn/libipnfw.c 
	bpv7/saga/saga.c 
	ici/libbloom-master/bloom.h 
	ici/libbloom-master/bloom.c 
	ici/libbloom-master/murmur2/MurmurHash2.c
	ici/crypto/NULL_SUITES/crypto.c
	ici/crypto/NULL_SUITES/csi.c
bpv7/cgr/libcgr.c
bpv7/utils/bpadmin.c
bpv7/utils/bpsecadmin.c
bpv7/ipn/ipnadmin.c
bpv7/dtn2/dtn2admin.c
bpv7/utils/lgsend.c
bpv7/utils/lgagent.c
bpv7/utils/bpstats.c
bpv7/utils/bptrace.c
bpv7/utils/bpcancel.c
bpv7/utils/bpversion.c
bpv7/utils/bplist.c
bpv7/utils/hmackeys.c
bpv7/dtn2/dtn2fw.c
bpv7/dtn2/dtn2adminep.c
bpv7/ipn/ipnfw.c
bpv7/ipn/ipnadminep.c
bpv7/ipnd/ipnd.c
bpv7/ipnd/helper.c
bpv7/ipnd/beacon.c
bpv7/ipnd/bpa.c
bpv7/bssp/bsspcli.c
bpv7/bssp/bsspclo.c
bpv7/ltp/ltpcli.c
bpv7/ltp/ltpclo.c
bpv7/tcp/tcpcli.c
bpv7/tcp/tcpclo.c
bpv7/stcp/stcpcli.c
bpv7/stcp/stcpclo.c
bpv7/udp/udpcli.c
bpv7/udp/udpclo.c
bpv7/test/bpsource.c
bpv7/test/bpsink.c
bpv7/test/bpdriver.c
bpv7/test/bpecho.c
bpv7/test/bpcounter.c
bpv7/utils/bpsendfile.c
bpv7/utils/bprecvfile.c
bpv7/test/bpcrash.c

bpv7/utils/cgrfetch.c
bpv7/daemon/bpclock.c
bpv7/daemon/bptransit.c
bpv7/daemon/bpclm.c
    );
# bibe, dccp, dgr, brs executables
# bpv7/test/bping.c  # FIXME

# These aren't ION_LWT ready
# bpv7/test/bpstats2.c
# bpv7/test/bpchat.c
} else {
    die "BPV6 Links TODO";
}

# Optional: ENABLE_IMC
# Skipped TC, DTKA, AMS, CFDP, BSS, BSSP, dTPC, ionrestart, ionexit


# Skipped: NM Manager not intended for embedded use
# TODO: Hard-coded for bpv7 ADMs.  This array should be split
my @nm_agent_sources = qw(
	nm/contrib/QCBOR/src/UsefulBuf.c 
	nm/contrib/QCBOR/src/qcbor_encode.c 
	nm/contrib/QCBOR/src/qcbor_decode.c 
	nm/contrib/QCBOR/src/ieee754.c
	nm/shared/adm/adm.c 
	nm/shared/msg/msg.c 
	nm/shared/primitives/ari.c 
	nm/shared/primitives/blob.c 
	nm/shared/primitives/ctrl.c 
	nm/shared/primitives/edd_var.c 
	nm/shared/primitives/expr.c 
	nm/shared/primitives/report.c 
	nm/shared/primitives/rules.c 
	nm/shared/primitives/table.c 
	nm/shared/primitives/tnv.c 
	nm/shared/utils/cbor_utils.c 
	nm/shared/utils/db.c 
	nm/shared/utils/nm_types.c 
	nm/shared/utils/rhht.c 
	nm/shared/utils/utils.c 
	nm/shared/utils/vector.c 
	nm/agent/ingest.c 
	nm/agent/instr.c 
	nm/agent/lcc.c 
	nm/agent/nmagent.c 
	nm/agent/ldc.c 
	nm/agent/rda.c 
	nm/shared/msg/ion_if.c 
	nm/agent/adm_amp_agent_impl.c 
	nm/agent/adm_amp_agent_agent.c 
	nm/shared/adm/adm_init.c
	bpv7/adm/adm_bp_agent_impl.c 
	bpv7/adm/adm_bp_agent_agent.c 
	bpv7/adm/adm_bpsec_impl.c 
	bpv7/adm/adm_bpsec_agent.c 
	bpv7/adm/adm_ion_bp_admin_impl.c 
	bpv7/adm/adm_ion_bp_admin_agent.c 
	nm/agent/adm_ion_admin_agent.c 
	nm/agent/adm_ion_admin_impl.c 
	nm/agent/adm_ionsec_admin_impl.c 
	nm/agent/adm_ionsec_admin_agent.c 
	nm/agent/adm_ion_ipn_admin_impl.c 
	nm/agent/adm_ion_ipn_admin_agent.c 
	nm/agent/adm_ion_ltp_admin_impl.c 
	nm/agent/adm_ion_ltp_admin_agent.c 
	nm/agent/adm_ltp_agent_impl.c 
	nm/agent/adm_ltp_agent_agent.c
    );

# NOTE: These files need significant modification to run in embedded systems
my @test_files = qw( tests/nm-unit/dotest.c );

if ($link_ion || $link_all) {
    parse_files(\@ici_sources);
    parse_files(\@ici_utils);
    parse_files(\@ltp_sources);
    #parse_files(\@dgr_sources);
    parse_files(\@bp_sources);
}

if ($link_nm || $link_all) {
    parse_files(\@nm_agent_sources);
    parse_files(\@test_files);
}
    
sub clean_links { # TODO
    # Open $tgt dir
    # For each file, unlink if -l
    # TODO: Delete unlink step from parse_files fn below, and call this before first parse_files
}
sub parse_files {
    my $sources = shift;
    foreach my $file (@${sources}) {
        my ($vol, $dir, $fn) = File::Spec->splitpath( $file );
        if (-l $fn) {
            unlink $fn;
        } elsif (-e $fn) {
            say "Warning: $fn already exists and is not a symlink";
            continue;
        }
        
        my $cmd = "ln -s $base/$file $tgt/";
        my $out = `$cmd`;
        say $out if $verbose && $out;
    }
}


