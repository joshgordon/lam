<?php
namespace LAM\ENVIRONMENT_CHECK;
use Exception;
use LAM\PERSISTENCE\ConfigurationDatabase;
use LAMCfgMain;
use function LAM\LOGIN\displayLoginHeader;

/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2010 - 2021  Roland Gruber

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/**
* Checks if the environment fits to run LAM.
* This code checks if the PHP version is ok, the required extensions are active
* and the memory limit is high enough.
*
* @author Roland Gruber
* @package main
*/

/** status messages */
include_once(__DIR__ . "/../lib/status.inc");

/** config */
include_once(__DIR__ . "/../lib/config.inc");

/** persistence */
include_once(__DIR__ . "/../lib/persistence.inc");

// check if PHP >= 7.4
if (version_compare(phpversion(), '7.4.0') < 0) {
	echo "<!DOCTYPE html>\n\n";
	echo "<html>\n<head>\n";
	echo "<meta name=\"robots\" content=\"noindex, nofollow\">\n";
	echo "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n";
	echo "<meta http-equiv=\"pragma\" content=\"no-cache\">\n		<meta http-equiv=\"cache-control\" content=\"no-cache\">\n";
	printHeaderContents('LDAP Account Manager', '..');
	echo "</head><body>\n";
	StatusMessage("ERROR", "LAM needs a PHP version which is greater or equal than 7.4.0.", "Please upgrade your PHP installation. The found version is " . phpversion());
	echo "<br><br>";
	echo "</body></html>";
	exit();
}

// check environment
$criticalErrors = array();
// check if PHP has session support
if (! function_exists('session_start')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no session support!", "Please install the session extension for PHP.");
}
// check if PHP has SPL support
if (! function_exists('class_implements')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no SPL support!", "Please install the SPL extension for PHP.");
}
// check if PHP has LDAP support
if (! function_exists('ldap_search')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no LDAP support!", "Please install the LDAP extension for PHP.");
}
// check if PHP has gettext support
if (! function_exists('gettext') || !function_exists('_')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no gettext support!", "Please install gettext for PHP.");
}
// check if PHP has openssl support
if (! function_exists('openssl_x509_parse')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no openssl support!", "Please install openssl for PHP.");
}
// check if PHP has XML support
if (! function_exists('utf8_decode') || !extension_loaded('xml')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no XML support!", "Please install the XML extension for PHP.");
}
if (!extension_loaded('libxml')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no Lib XML support!", "Please install the Lib XML extension for PHP.");
}
if (!class_exists('XMLReader')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no XMLReader support!", "Please install XMLReader for PHP.");
}
if (!class_exists('XMLWriter')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no XMLWriter support!", "Please install XMLWriter for PHP.");
}
if (!function_exists('mb_strlen')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no MB String support!", "Please install the MB String extension for PHP.");
}
// imagick/GD
if (!extension_loaded('imagick') && !extension_loaded('gd')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no imagick or GD support.", "Please install the imagick or GD extension for PHP.");
}
// check if PHP has JSON support
if (!function_exists('json_encode')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no JSON support!", "Please install the JSON extension for PHP.");
}
// check if PHP has GMP support
if (!extension_loaded('gmp')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no GMP support!", "Please install the GMP extension for PHP.");
}
// check if PHP has filter support
if (!extension_loaded('filter')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no filter support!", "Please install the filter extension for PHP.");
}
// check file permissions
$writableDirs = array('sess', 'tmp');
for ($i = 0; $i < sizeof($writableDirs); $i++) {
	$path = realpath('../') . "/" . $writableDirs[$i];
	if (!is_writable($path)) {
		$criticalErrors[] = array("ERROR", 'The directory %s is not writable for the web server. Please change your file permissions.', '', array($path));
	}
}
// check session auto start
if (ini_get("session.auto_start") == "1") {
	$criticalErrors[] = array("ERROR", "Please deactivate session.auto_start in your php.ini. LAM will not work if it is activated.");
}
// check memory limit
$memLimit = ini_get('memory_limit');
if (isset($memLimit)
	&& ($memLimit != '')
	&& (substr(strtoupper($memLimit), strlen($memLimit) - 1) == 'M')
	&& (intval(substr($memLimit, 0, strlen($memLimit) - 1)) < 128)) {
	$criticalErrors[] = array("ERROR", "Please increase the \"memory_limit\" parameter in your php.ini to at least \"128M\".",
		"Your current memory limit is $memLimit.");
}
// check PCRE regex system
if (!@preg_match('/^\p{L}+$/u', "abc")) {
	$criticalErrors[] = array("ERROR", "Your PCRE library has no complete Unicode support. Please upgrade libpcre or compile with \"--enable-unicode-properties\".");
}
// check ZIP support for PDF files in file upload
if (!extension_loaded('zip')) {
	$criticalErrors[] = array("ERROR", "Your PHP has no ZIP support.", "Please install the ZIP extension for PHP.");
}
// check for main config
try {
	$mainConfig = new LAMCfgMain();
	if (!$mainConfig->isConfigFileExisting() && !$mainConfig->installSampleConfig()) {
		$criticalErrors[] = array("ERROR", "The main config file (config.cfg) does not exist.", "Please see the manual for installation instructions.");
	}
	$mainConfig = new LAMCfgMain();
	$configurationErrors = array();
	$configurationDatabase = new ConfigurationDatabase($mainConfig);
	$configurationDatabase->checkConnection();
}
catch (Exception $e) {
	logNewMessage(LOG_ERR, 'Unable to connect to configuration database: ' . $e->getMessage());
	$configurationErrors[] = array('ERROR', _('Unable to connect to configuration database.'));
}
// stop login if errors occurred
if (!empty($criticalErrors) || !empty($configurationErrors)) {
	echo "<!DOCTYPE html>\n\n";
	echo "<html>\n<head>\n";
	echo "<meta name=\"robots\" content=\"noindex, nofollow\">\n";
	echo "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n";
	echo "<meta http-equiv=\"pragma\" content=\"no-cache\">\n";
	echo "<meta http-equiv=\"cache-control\" content=\"no-cache\">\n";
	printHeaderContents('LDAP Account Manager', '..');
	echo "</head><body>\n";
	if (empty($criticalErrors)) {
		displayLoginHeader();
	}
	foreach ($criticalErrors as $criticalError) {
		call_user_func_array("StatusMessage", $criticalError);
		echo "<br><br>";
	}
	foreach ($configurationErrors as $configurationError) {
		call_user_func_array("StatusMessage", $configurationError);
		echo "<br><br>";
	}
	echo "</body></html>";
	exit();
}
