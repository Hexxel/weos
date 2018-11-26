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

* Basis             supports 4.x and only 5.1.x
* Corazon           Supports only 4.x
* Coronet-Viper     Supoorts only 4.x
* Coronet Viper-TBN Supports only 5.x
* Dagger-RFIR       Supports only 5.x
* Dagger-Lynx       Supports only 5.x

## Required tests

The following tests **must** be performed for each product family with success or
the commit may be reverted.  

From barebox:

* 1.1 Boot primary. (See list above)

* 1.2 Boot secondary. (See list above)

* 1.3 Boot net (BOOTP). (See list above)

* 1.4 Boot system recovery. (Test with dhcp and static ip)

* 1.5 Check MTD partitions (all is present and accessible):
            md -s /dev/primary
            md -s /dev/secondary
            md -s /dev/config
            md -s /dev/etc

* 1.6 Upgrade barebox from barebox. Check that detection of wrong file type works.
      I.e. Corazon barebox file does not upgrades on a Coronet device.

* 1.7 Compare the primary with secondary partition (upgrade the same version on both partition):
      memcmp -s /dev/primary -d /dev/secondary 0 0 <size> <size>

* 1.8 Make a checksum of the image:
      md5sum -s /dev/primary 0x+<size>, md5sum -s /dev/secondary 0x+<size>


From WeOS:

* 2.1 Ports are working in WeOS and is able to ping. Test one port per port type.
* 2.2 Upgrade primary from WeOS with reboot. See the list above for WeOS images.
* 2.3 Upgrade secondary from WeOS with reboot. See the list above for WeOS images.
* 2.4 Upgrade boot from WeOS with reboot

Barebox and WeOS combined:

* 3.1 Enable loglevel=8 in barebox and check boot sequence (console and dmesg) for
unexpected output using a previous version of barebox as baseline.
(edit /env/etc/config -> export global.loglevel=8)

Build system:

* 4.1 All supported builds are building (no new warnings)
* 4.2 Release script (release.sh) is working
