#!/usr/bin/perl 
###########################################
#
# this script will parse the db generated by Gatherer Extractor to build a
#	file with Wagic card definitions partially filled in.
############################################
my %rarityMap = ( 
			"Uncommon" => "U", 
			"Common" => "C",
			"Rare" 	=> "R",
			"Mythic" => "M"
		);

my %costNumberMap = (	
			"a" => 1,
			"an" => 1,
			"one" => 1,
			"two" => 2,
			"three" => 3,
			"four" => 4
		);
my %knownCards = ();
getKnownCards(\%knownCards);

my %extraCosts = ();
my $cardCost;
while (<>)
{
	chomp;
	s///g;
	s/\x97/\-\-/g;
	s/\x92/'/g;
	s/\xC6/AE/g;
	s/\xF6/o/g; # o with two dots
	s/\xE8/e/g; # for Seance
	s/\xE9/e/g; # for Seance
	s/Text=/text=/;

	next if (/\[card/ );

	if (/\[\/card/ )
	{
		foreach $extraCost (keys %extraCosts)
		{
			my $cost = $extraCost eq "Multikicker" ? "multi" : "";
			print lc ($extraCost) ;
			print "=$cost" . subCosts($extraCosts{$extraCost}) . "\n";
			delete $extraCosts{$extraCost};
		}
		print "[/card]\n" if (!$skip);
		$skip = 0;
	}
	elsif ($skip == 1)
	{
		next;;
	}
	elsif (/^Name=(.*)/)
	{
		my $cardName = $1;
		$cardName =~ s/^\s*|\s*$//g;
		if ($cardName =~ /\((.*?)\)/ )
		{
			my $m = $1;
			my $pre = $`;
			$m =~ s/^\s*|\s*$//g;
			$pre =~ s/^\s*|\s*$//g;
			if ( ($knownCards{$m} == 1)  || ($knownCards{$pre} == 1) )
			{
				$skip = 1;
				print STDERR "*$cardName\n";
				next;
			}
			else
			{
			#	print STDERR "No Match. $cardName\n";	
			}
		}	
		if ($knownCards{$cardName} == 1)
		{
			$skip = 1;			
			print STDERR "$cardName\n";
		}
		else
		{
			print "[card]\nname=$cardName\n";
		}
	}
	elsif (/Mana=/)
	{
		s/Mana=//;
		s/(\w)/\{$1}/g;
		s/\((\{.*?\})\)/$1/g;
		s/\}\/\{/\//g;
		print "mana=$_\n";
		$cardCost = $_;
	}

	elsif (/^Type=/ )
	{
		my ($type, $subtype) = split /\s*\-\-\s*/, $';
		print "type=$type\n";
		print "subtype=$subtype\n" if ($subtype);
	}
	elsif (/^Power/)
	{
		s/Power.*?=//gi;
		s/[\(\)]//g;
		my ($power, $toughness) = split /\//;	
		print "power=$power\n" if ($power);
		print "toughness=$toughness\n" if ($toughness);
	}
	elsif (/Rarity=/)
	{
		my @setsInfo = split /,/, $';
		my @sets, @rarity;
		foreach my $setInfo (@setsInfo)
		{
			my ($set, $rarity);
			if ($setInfo  =~ /(.*?)(Rare|Common|Uncommon|Mythic)/i)
			{
				$set = $1;
				$rarity = $rarityMap{$2};
			}
		}	
	}
	elsif ( /(Multikicker|Kicker|Buyback|Flashback)\s*\-\-?\s*(.*?)\(You may/)
	{
		print "$_\n";
		$extraCosts{$1} = $2;
		#print STDERR "** $1=$2\n";	
	}
	elsif ( /(Kicker|Flashback|Buyback).*?(\{[\}\{GUBWRP\d].*?)\(/ )
	{
		print "$_\n";
		$extraCosts{$1} = $2;
		#print STDERR "+ $1=$2\n";	
	}
	elsif ( /^(Retrace)/ )
	{
		$extraCosts{$1} = $cardCost;
		s/Text=/text=/;
		print "$_\n";
	}
	elsif ( /Text=/ )
	{
		print "text=$'\n";
	}
	else
	{
		print "$_\n";
	}

}


sub subCosts
{
	my $cost = shift (@_);
	my $discardText = '{discard(*|myhand)}';
	my $sacIslandText = '{S(island|mybattlefield)}';
	my $sacLandText = '{S(land|mybattlefield)}';
	my $sacCreatureText = '{S(creature|mybattlefield)}';
	my $tapText = '{S(creature|mybattlefield)}';
	$cost =~ s/Pay (\d+) life/'{L}' x $1/ei;
	$cost =~ s/Discard (a|one|two|three) cards? at random.*/{D}{$costNumberMap{$1}}/ei;
	$cost =~ s/Discard (one|two|three) cards?.*/$discardText x $costNumberMap{$1}/ei;
	$cost =~ s/Sacrifice (one|two|three) islands/$sacIslandText x $costNumberMap{$1}/ei;
	$cost =~ s/Sacrifice (a|one|two|three) lands.*/$sacLandText x $costNumberMap{$1}/ei;
	$cost =~ s/Sacrifice (a|one|two|three) creature.*/$sacCreatureText x $costNumberMap{$1}/ei;
	$cost =~ s/Tap (an|one|two|three) untapped (.*?) you control.*/"{T($2|mybattlefield)}" x $costNumberMap{$1}/ei;
	$cost =~ s/Return (a|one|two|three) creature(.*?) .*/"{H(creature|mybattlefield)}" x $costNumberMap{$1}/ei;

	return $cost;	
}


sub getKnownCards
{
	my $m = shift (@_);
	open INFILE,"<../miki/supported_cards.txt" or die "Can't open file! $!\n";
	my @cards = <INFILE>;
	foreach (@cards)
	{
		chomp;
		s///g;
		s/^\s*|\s*$//g;
		next if (/Token$/);
		$m->{$_} = 1;
	}
}
