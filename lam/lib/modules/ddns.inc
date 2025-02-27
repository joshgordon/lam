<?php
/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2008         Thomas Manninger
                2008 - 2022  Roland Gruber

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
* Manages DDNS entries.
*
* @package modules
*
* @author Thomas Manninger
* @author Roland Gruber
*/

/**
* Manages DDNS entries.
*
* @package modules
*/

class ddns extends baseModule {

	/** standard DDNS settings */
	public $ddns = array();


	/**
	* Returns true if this module can manage accounts of the current type, otherwise false.
	*
	* @return boolean true if module fits
	*/
	public function can_manage() {
		return in_array($this->get_scope(), array('dhcp'));
	}

	/**
	 * Returns meta data that is interpreted by parent class.
	 *
	 * @return array array with meta data
	 */
	public function get_metaData() {
		$return = array();
		// alias name
		$return["alias"] = _("DDNS");
		// this is a base module
		$return["is_base"] = false;
		// icon
		$return['icon'] = 'world-color.svg';
		// RDN attribute
		$return["RDN"] = array("cn" => "high");
		// LDAP filter
		$return["ldap_filter"] = array();
		// module dependencies
		$return['dependencies'] = array('depends' => array('dhcp_settings'), 'conflicts' => array());
		// managed object classes
		$return['objectClasses'] = array();
		// managed attributes
		$return['attributes'] = array('dhcpOption', 'dhcpStatements');
		// help Entries
		$return['help'] = array(
			'active' => array(
				"Headline" => _("Activate DynDNS"),
				"Text" => _("Should DDNS (Dynamic DNS) be activated?")
			),
			'fixed_ips' => array(
				"Headline" => _("Fix IP addresses"),
				"Text" => _("Should fix IP addresses be added to the DNS server?")
			),
			'client_insert' => array(
				"Headline" => _("Disable client updates"),
				"Text" => _("Disables the client to update DNS entries.")
			),
			'keypath' => array(
				"Headline" => _("Path to key for DNS updates"),
				"Text" => _("The key enables the DHCP server to perform DNS updates." .
				" " . "The key is generated with \"genDDNSkey\".")
			),
			'dns' => array(
				"Headline" => _("IP address of the DNS server"),
				"Text" => _("Please enter the IP address of your DNS server.")
			),
			'zone' => array(
				"Headline" => _("Zone name"),
				"Text" => _("Zone name for the DNS server (e.g. company.local).")
			),
			'zone_reverse' => array(
				"Headline" => _("Reverse zone name"),
				"Text" => ("Name of the reverse zone of the DNS server (e.g. 0.168.192.in-addr.arpa).")
			),
		);
		// available PDF fields
		$return['PDF_fields'] = array(
			'DNSserver' => _('IP address of the DNS server'),
			'zone' => _('Zone name'),
			'reverseZone' => _('Reverse zone name'),
		);
		// upload fields
		if (isLoggedIn() && $this->check_if_ddns_is_enable()) {
			$return['upload_columns'] = array(
				array(
					'name' => 'ddns_DNSserver',
					'description' => _('IP address of the DNS server'),
					'help' => 'dns',
					'example' => '123.123.123.123',
					'required' => true
				),
				array(
					'name' => 'ddns_zone',
					'description' => _('Zone name'),
					'help' => 'zone',
					'example' => 'company.local',
					'required' => true
				),
				array(
					'name' => 'ddns_reverseZone',
					'description' => _('Reverse zone name'),
					'help' => 'zone_reverse',
					'example' => '0.168.192.in-addr.arpa',
					'required' => true
				),
			);
		}
		return $return;
	}

	/**
	* This function fills the message array.
	*/
	public function load_Messages() {
		$this->messages['key_path'][0] = array('ERROR', 'Please enter the path to the key.', '');
		$this->messages['key_path'][1] = array('ERROR', 'The key path contains invalid characters.', '');
		$this->messages['ip'][0] = array('ERROR', 'The IP address of the DNS server is invalid.');
		$this->messages['ip'][1] = array('ERROR', _('Account %s:') . ' ddns_DNSserver', 'The IP address of the DNS server is invalid.');
		$this->messages['zone'][0] = array('ERROR', 'Please enter a zone name.');
		$this->messages['zone_reverse'][0] = array('ERROR', 'Please enter the reverse zone.');
	}

	/**
	* This functions returns true if all needed settings are done.
	*
	* @return boolean true if LDAP operation can be done
	*/
	public function module_complete() {
		$this->attributes = &$this->getAccountContainer()->getAccountModule('dhcp_settings')->attributes;
		$this->orig = &$this->getAccountContainer()->getAccountModule('dhcp_settings')->orig;
		if ($this->isRootNode()) {
			// check if DHCP main settings and valid DHCP entry
			if (!in_array_ignore_case('dhcpService', $this->attributes['objectClass']) && !in_array_ignore_case('dhcpServer', $this->attributes['objectClass'])) {
				return false;
			}
		}
		else {
			if (!$this->check_if_ddns_is_enable()) {
				return true;
			}
			// Account settings
			$ip = $this->getDNSServer();
			if (!empty($ip) && !check_ip($ip)) return false;

			$zones = $this->getZoneNames();
			if (sizeof($zones) < 2) {
				return false;
			}
		}

		return true;
	}

	/**
	 * Returns if the current DN is the root entry.
	 *
	 * @return bool is root
	 */
	private function isRootNode() {
		$rootSuffix = $this->getAccountContainer()->get_type()->getSuffix();
		return $this->getAccountContainer()->dn_orig == $rootSuffix;
	}

	/**
	* Returns a list of modifications which have to be made to the LDAP account.
	*
	* @return array list of modifications
	* <br>This function returns an array with 3 entries:
	* <br>array( DN1 ('add' => array($attr), 'remove' => array($attr), 'modify' => array($attr)), DN2 .... )
	* <br>DN is the DN to change. It may be possible to change several DNs (e.g. create a new user and add him to some groups via attribute memberUid)
	* <br>"add" are attributes which have to be added to LDAP entry
	* <br>"remove" are attributes which have to be removed from LDAP entry
	* <br>"modify" are attributes which have to been modified in LDAP entry
	* <br>"info" are values with informational value (e.g. to be used later by pre/postModify actions)
	*/
	public function save_attributes() {
		// done by dhcp_server object
	}

	/**
	 * This function check if ddns is enable.
	 */
	private function check_if_ddns_is_enable() {
		if ($this->getAccountContainer() == null) {
			return false;
		}
		$type = $this->getAccountContainer()->get_type();
		if ($type == null) {
			return false;
		}
		$suffix = $type->getSuffix();

		$results = searchLDAP($suffix, "dhcpStatements=ddns-update-style interim", array('dn'));
		return (!empty($results));
	}

	/**
	* Processes user input of the primary module page.
	* It checks if all input values are correct and updates the associated LDAP attributes.
	*
	* @return array list of info/error messages
	*/
	public function process_attributes() {
		$errors = array();

		// Main Settings and Account have to different processes.
		if ($this->isRootNode()) {
			// main settings:
			$errors = $this->process_attributes_mainSettings();
		}
		else {
			// account
			if (!$this->check_if_ddns_is_enable()) {
				return array();
			}

			$errors = $this->process_attributes_account();
		}

		return $errors;
	}

	/**
	 * Process for mainsettings
	 */
	public function process_attributes_mainSettings() {
		if (!in_array_ignore_case('dhcpService', $this->attributes['objectClass']) && !in_array_ignore_case('dhcpServer', $this->attributes['objectClass'])) {
			return array();
		}
		$errors = array();

		// Is DDNS active?
		$active = $_POST['active'];

		// Insert fixed IPs into DNS?
		$insert_fixed = $_POST['insert_fixed'];

		// Client can contribute which is registered into DNS
		$client_insert = $_POST['client_insert'];

		// The path to the key:
		$key_path = trim($_POST['key_path']);

		$this->setDynDNSActivated(($active == 'on'));

		$this->setFixIPs(($insert_fixed == 'on'));

		$this->setIgnoreClientUpdates(($client_insert == 'on'));

		$this->setUpdateKey($key_path);
		if (!empty($key_path) && (str_replace("\"","",$_POST['key_path']) != $key_path)) {
			$errors[] = $this->messages['key_path'][1];
		}

		return $errors;
	}

	/**
	 * Process for account
	 */
	public function process_attributes_account() {
		$errors = array();

		$ip = trim($_POST['ip']);
		$zone = trim($_POST['zone']);
		$zone_reverse = trim($_POST['zone_reverse']);

		// ip correct???
		if (!empty($ip) && !check_ip($ip)) {
			$errors[] = $this->messages['ip'][0];
		}

		for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
			if (substr($this->attributes['dhcpStatements'][$i], 0, 5) == 'zone ') {
				unset($this->attributes['dhcpStatements'][$i]);
				$this->attributes['dhcpStatements'] = array_values($this->attributes['dhcpStatements']);
				$i--;
			}
		}

		// Zone inserted?
		if (!empty($zone)) {
			if (!empty($ip)) {
				$this->attributes['dhcpStatements'][] = "zone {$zone}. { primary {$ip}; key DHCP_UPDATER; }";
			}
			else {
				$this->attributes['dhcpStatements'][] = "zone {$zone}. { key DHCP_UPDATER; }";
			}
		}
		else {
			if (!empty($ip)) {
				$errors[] = $this->messages['zone'][0];
			}
		}

		// Zone reverse inserted?
		if (!empty($zone_reverse)) {
			if (!empty($ip)) {
				$this->attributes['dhcpStatements'][] = "zone {$zone_reverse}. { primary {$ip}; key DHCP_UPDATER; }";
			}
			else {
				$this->attributes['dhcpStatements'][] = "zone {$zone_reverse}. { key DHCP_UPDATER; }";
			}
		}
		else {
			if (!empty($ip)) {
				$errors[] = $this->messages['zone_reverse'][0];
			}
		}
		return $errors;
	}

	/**
	 * Returns the HTML meta data for the main account page.
	 *
	 * @return htmlElement HTML meta data
	 */
	public function display_html_attributes() {
		$this->attributes = &$this->getAccountContainer()->getAccountModule('dhcp_settings')->attributes;
		$this->orig = &$this->getAccountContainer()->getAccountModule('dhcp_settings')->orig;
		$return = new htmlResponsiveRow();
		// check if DHCP main settings and valid DHCP entry
		if ($this->isRootNode()) {
			if (!in_array_ignore_case('dhcpService', $this->attributes['objectClass']) && !in_array_ignore_case('dhcpServer', $this->attributes['objectClass'])) {
				$return->addElement(new htmlStatusMessage('ERROR', _('Please set your LDAP suffix to an LDAP entry with object class "dhcpService" or "dhcpServer".')));
				return $return;
			}
			// DHCP main settings
			$return->add(new htmlResponsiveInputCheckbox('active', $this->isDynDNSActivated(), _('Activate DynDNS'), 'active'), 12);
			$return->add(new htmlResponsiveInputCheckbox('insert_fixed', $this->addFixIPs(), _('Add fix IP addresses to DNS'), 'fixed_ips'), 12);
			$return->add(new htmlResponsiveInputCheckbox('client_insert', $this->isIgnoreClientUpdates(), _('Disable client updates'), 'client_insert'), 12);
			$keyInput = new htmlResponsiveInputField(_('Path to key for DNS updates'), 'key_path', $this->getUpdateKey(), 'keypath');
			$return->add($keyInput, 12);
		}
		else {
			// Account edit
			if (!$this->check_if_ddns_is_enable()) {
				$return->add(new htmlOutputText(_("DDNS is not activated. You can activate it in the DHCP settings (DDNS).")), 12);
			}
			else {
				// DNS server
				$serverInput = new htmlResponsiveInputField(_('IP address of the DNS server'), 'ip', $this->getDNSServer(), 'dns');
				$return->add($serverInput, 12);

				$zones = $this->getZoneNames();
				$zone = '';
				$revzone = '';
				if (isset($zones[0])) {
					$zone = $zones[0];
				}
				if (isset($zones[1])) {
					$revzone = $zones[1];
				}
				// zone name
				$zoneInput = new htmlResponsiveInputField(_('Zone name'), 'zone', $zone, 'zone');
				$return->add($zoneInput, 12);
				// reverse zone name
				$revZoneInput = new htmlResponsiveInputField(_('Reverse zone name'), 'zone_reverse', $revzone, 'zone_reverse');
				$return->add($revZoneInput, 12);
			}
		}

		return $return;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfEntries()
	 */
	public function get_pdfEntries($pdfKeys, $typeId) {
		// attributes are taken from DHCP server object
		$this->attributes = &$this->getAccountContainer()->getAccountModule('dhcp_settings')->attributes;
		$this->orig = &$this->getAccountContainer()->getAccountModule('dhcp_settings')->orig;
		$zones = $this->getZoneNames();
		$zone = '';
		$revzone = '';
		if (isset($zones[0])) {
			$zone = $zones[0];
		}
		if (isset($zones[1])) {
			$revzone = $zones[1];
		}
		$result = array();
		$this->addPDFKeyValue($result, 'DNSserver', _('IP address of the DNS server'), $this->getDNSServer());
		$this->addPDFKeyValue($result, 'zone', _('Zone name'), $zone);
		$this->addPDFKeyValue($result, 'reverseZone', _('Reverse zone name'), $revzone);
		return $result;
	}

	/**
	 * Returns the IP of the DNS server.
	 *
	 * @return String IP address
	 */
	private function getDNSServer() {
		$return = null;
		if (isset($this->attributes['dhcpStatements'][0])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if (substr($this->attributes['dhcpStatements'][$i], 0, 5) == 'zone ') {
					if (strpos($this->attributes['dhcpStatements'][$i], ' primary ') === false) {
						continue;
					}
					$parts = explode(". { primary ", $this->attributes['dhcpStatements'][$i]);
					$temp = array_pop($parts);
					$temp = explode(";", $temp);
					return array_shift($temp);
				}
			}
		}
		return $return;
	}

	/**
	 * Returns the zone names.
	 *
	 * @return array zone names array(zone, reverse zone)
	 */
	private function getZoneNames() {
		$return = array();
		$zone = '';
		$revZone = '';
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if (substr($this->attributes['dhcpStatements'][$i], 0, 5) == 'zone ') {
					$parts = explode(" ",substr($this->attributes['dhcpStatements'][$i],5));
					$value = substr(array_shift($parts),0,-1);
					if (strpos($value, 'in-addr.arpa') === false) {
						$zone = $value;
					}
					else {
						$revZone = $value;
					}
				}
			}
		}
		$return[0] = $zone;
		$return[1] = $revZone;
		return $return;
	}

	/**
	 * Returns if DDNS is activated.
	 *
	 * @return boolean activated
	 */
	private function isDynDNSActivated() {
		$return = false;
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if ($this->attributes['dhcpStatements'][$i] == 'ddns-update-style interim') {
					$return = true;
					break;
				}
			}
		}
		return $return;
	}

	/**
	 * Sets if DDNS is activated.
	 *
	 * @param boolean $activated activated
	 */
	private function setDynDNSActivated($activated) {
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if (substr($this->attributes['dhcpStatements'][$i], 0, 18) == 'ddns-update-style ') {
					unset($this->attributes['dhcpStatements'][$i]);
					$this->attributes['dhcpStatements'] = array_values($this->attributes['dhcpStatements']);
				}
			}
		}
		if ($activated) {
			$this->attributes['dhcpStatements'][] = 'ddns-update-style interim';
		}
		else {
			$this->attributes['dhcpStatements'][] = 'ddns-update-style none';
		}
	}

	/**
	 * Returns if fixed IPs are added to DDNS.
	 *
	 * @return boolean add fixed IPs
	 */
	private function addFixIPs() {
		$return = false;
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if ($this->attributes['dhcpStatements'][$i] == 'update-static-leases true') {
					$return = true;
					break;
				}
			}
		}
		return $return;
	}

	/**
	 * Sets if client updates are ignored.
	 *
	 * @param boolean $add add fixed IPs
	 */
	private function setFixIPs($add) {
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if (substr($this->attributes['dhcpStatements'][$i], 0, 21) == 'update-static-leases ') {
					unset($this->attributes['dhcpStatements'][$i]);
					$this->attributes['dhcpStatements'] = array_values($this->attributes['dhcpStatements']);
				}
			}
		}
		if ($add) {
			$this->attributes['dhcpStatements'][] = 'update-static-leases true';
		}
	}

	/**
	 * Returns if client updates are ignored.
	 *
	 * @return boolean ignore client updates
	 */
	private function isIgnoreClientUpdates() {
		$return = false;
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if (preg_replace('/[ ]+/', ' ', $this->attributes['dhcpStatements'][$i]) == 'ignore client-updates') {
					$return = true;
					break;
				}
			}
		}
		return $return;
	}

	/**
	 * Sets if client updates are ignored.
	 *
	 * @param boolean $ignore ignore client updates
	 */
	private function setIgnoreClientUpdates($ignore) {
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if (preg_replace('/[ ]+/', ' ', $this->attributes['dhcpStatements'][$i]) == 'ignore client-updates') {
					if ($ignore) {
						return;  // option already set, no change
					}
					unset($this->attributes['dhcpStatements'][$i]);
					$this->attributes['dhcpStatements'] = array_values($this->attributes['dhcpStatements']);
				}
			}
		}
		if ($ignore) {
			$this->attributes['dhcpStatements'][] = 'ignore client-updates';
		}
	}

	/**
	 * Returns the key for DNS updates.
	 *
	 * @return String key
	 */
	private function getUpdateKey() {
		$return = null;
		if (is_array($this->attributes['dhcpStatements'])) {
			for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
				if (substr($this->attributes['dhcpStatements'][$i], 0, 8) == 'include ') {
					$return = substr($this->attributes['dhcpStatements'][$i],9, strlen($this->attributes['dhcpStatements'][$i]) - 10);
					break;
				}
			}
		}
		return $return;
	}

	/**
	 * Sets the key for DNS updates.
	 *
	 * @param String $key key
	 */
	private function setUpdateKey($key) {
		if (!is_array($this->attributes['dhcpStatements'])) {
			$this->attributes['dhcpStatements'] = array();
		}
		for ($i = 0; $i < sizeof($this->attributes['dhcpStatements']); $i++) {
			if (substr($this->attributes['dhcpStatements'][$i], 0, 8) == 'include ') {
				unset($this->attributes['dhcpStatements'][$i]);
				$this->attributes['dhcpStatements'] = array_values($this->attributes['dhcpStatements']);
			}
		}
		if (($key != null) && ($key != '')) {
			$this->attributes['dhcpStatements'][] = 'include "' . $key . '"';
		}
	}

	/**
	* This function loads the LDAP attributes when an account should be loaded.
	*
	* Calling this method requires the existence of an enclosing {@link accountContainer}.<br>
	* <br>
	* By default this method loads the object classes and accounts which are specified in {@link getManagedObjectClasses()}
	* and {@link getManagedAttributes()}.
	*
	* @param array $attributes array like the array returned by get_ldap_attributes(dn of account) but without count indices
	*/
	public function load_attributes($attributes) {
		// load nothing, attributes are saved in "dhcp_settings" module
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::build_uploadAccounts()
	 */
	function build_uploadAccounts($rawAccounts, $ids, &$partialAccounts, $selectedModules, &$type) {
		$messages = array();
		if (!$this->check_if_ddns_is_enable()) {
			return $messages;
		}
		for ($i = 0; $i < sizeof($rawAccounts); $i++) {
			// principal name
			if (!check_ip($rawAccounts[$i][$ids['ddns_DNSserver']])) {
				$error = $this->messages['ip'][1];
				array_push($error, array($i));
				$messages[] = $error;
			}
			else {
				$partialAccounts[$i]['dhcpStatements'][] = "zone {$rawAccounts[$i][$ids['ddns_zone']]}. { primary {$rawAccounts[$i][$ids['ddns_DNSserver']]}; key DHCP_UPDATER; }";
				$partialAccounts[$i]['dhcpStatements'][] = "zone {$rawAccounts[$i][$ids['ddns_reverseZone']]}. { primary {$rawAccounts[$i][$ids['ddns_DNSserver']]}; key DHCP_UPDATER; }";
			}
		}
		return $messages;
	}

}

?>
