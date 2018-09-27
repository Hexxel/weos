# Barebox test requirements

## Preamble

To increase the quality of barebox and minimize unnecessary work this document
specifices a number of test that **must** be performed before a merge to relevant
master is done according to QMS085 and QMS044 (where applicable).
Because the number of supported boards and product families is increasing
the tests outlined in this document is deemed the minimum acceptable tests done in
abscence of a dedicated barebox test system.

## Product families

For this version of barebox we currently support the following:

* Basis
* Corazon
* Coronet
* Dagger

## Required tests

The following tests **must** be performed for each product family with success or
the commit may be reverted.  

From barebox:

* 1.1 Boot primary
* 1.2 Boot secondary
* 1.3 Boot net (BOOTP)
* 1.4 Boot rescue
* 1.5 Check MTD partitions (all is present and accessible)

From WeOS:

* 2.1 Ports are working in WeOS and is able to ping
* 2.2 Upgrade primary from WeOS with reboot
* 2.3 Upgrade secondary from WeOS with reboot
* 2.4 Upgrade boot from WeOS with reboot

Barebox and WeOS combined:

* 3.1 Enable loglevel=8 in barebox and check boot sequence (console and dmesg) for
unexpected output using a previous version of barebox as baseline

Build system:

* 4.1 All supported builds are building (no new warnings)
* 4.2 Release script (release.sh) is working
