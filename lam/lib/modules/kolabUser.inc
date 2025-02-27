<?php
use \LAM\PDF\PDFTable;
use \LAM\PDF\PDFTableCell;
use \LAM\PDF\PDFTableRow;
/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2004 - 2022  Roland Gruber

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
* Manages Kolab user accounts.
*
* @package modules
* @author Roland Gruber
*/

/**
* Manages Kolab user accounts.
*
* @package modules
*/
class kolabUser extends baseModule {

	/** list of invitation policies */
	private $invitationPolicies;

	/**
	* Creates a new kolabUser object.
	*
	* @param string $scope account type (user, group, host)
	*/
	function __construct($scope) {
		// call parent constructor
		parent::__construct($scope);
		// list of invitation policies
		$this->invitationPolicies = array(
			'ACT_ALWAYS_ACCEPT' => _('Always accept'),
			'ACT_ALWAYS_REJECT' => _('Always reject'),
			'ACT_MANUAL' => _('Manual'),
			'ACT_REJECT_IF_CONFLICTS' => _('Reject if conflicts'),
			'ACT_MANUAL_IF_CONFLICTS' => _('Manual if conflicts')
		);
		$this->autoAddObjectClasses = false;
	}

	/**
	* Returns true if this module can manage accounts of the current type, otherwise false.
	*
	* @return boolean true if module fits
	*/
	public function can_manage() {
		return in_array($this->get_scope(), array('user'));
	}

	/**
	* Returns meta data that is interpreted by parent class
	*
	* @return array array with meta data
	*
	* @see baseModule::get_metaData()
	*/
	function get_metaData() {
		$return = array();
		// icon
		$return['icon'] = 'kolab.png';
		// alias name
		$return["alias"] = _("Kolab");
		// module dependencies
		$return['dependencies'] = array('depends' => array('inetOrgPerson'), 'conflicts' => array());
		// LDAP filter
		$return["ldap_filter"] = array('or' => "(objectClass=kolabInetOrgPerson)");
		// managed object classes
		$return['objectClasses'] = array('kolabInetOrgPerson', 'mailrecipient');
		// managed attributes
		$return['attributes'] = array('alias', 'kolabDelegate', 'kolabInvitationPolicy',
										'kolabAllowSMTPRecipient', 'kolabAllowSMTPSender', 'mailQuota');
		// profile options
		$profileContainer = new htmlResponsiveRow();
		$profileContainer->add(new htmlResponsiveInputField(_('Mailbox quota'), 'kolab_mailQuota', null, 'mailQuota'), 12);
		$return['profile_options'] = $profileContainer;
		// profile checks
		$return['profile_checks']['kolab_mailQuota'] = array(
			'type' => 'ext_preg',
			'regex' => 'digit',
			'error_message' => $this->messages['mailQuota'][0]);
		// self service field settings
		$return['selfServiceFieldSettings'] = array(
			'kolabDelegate' => _('Delegates'),
			'kolabInvitationPolicy' => _('Invitation policy')
		);
		// possible self service read-only fields
		$return['selfServiceReadOnlyFields'] = array('kolabDelegate', 'kolabInvitationPolicy');
		// help Entries
		$return['help'] = array(
			'invPol' => array(
				"Headline" => _("Invitation policy"), 'attr' => 'kolabInvitationPolicy',
				"Text" => _("For automatic invitation handling.")
			),
			'invPolList' => array(
				"Headline" => _("Invitation policy list"), 'attr' => 'kolabInvitationPolicy',
				"Text" => _("This is a comma separated list of invitation policies.")
			),
			'delegate' => array(
				"Headline" => _("Delegates"), 'attr' => 'kolabDelegate',
				"Text" => _("A user may define who is allowed to act on behalf of herself. This property is checked when using the Kolab smtp daemon (Postfix) to send emails.")
			),
			'delegateList' => array(
				"Headline" => _("Delegates"), 'attr' => 'kolabDelegate',
				"Text" => _("This is a semi-colon separated list of delegates.")
			),
			'alias' => array(
				"Headline" => _("Email alias"), 'attr' => 'alias',
				"Text" => _("Email alias for this account.")
			),
			'aliasList' => array(
				"Headline" => _("Email alias list"), 'attr' => 'alias',
				"Text" => _("This is a comma separated list of eMail aliases.")
			),
			'kolabAllowSMTPRecipient' => array (
				"Headline" => _('Allowed recipients'), 'attr' => 'kolabAllowSMTPRecipient',
				"Text" => _('Describes the allowed or disallowed SMTP recipient addresses for mail sent by this account (e.g. "domain.tld" or "-user@domain.tld").')
			),
			'kolabAllowSMTPRecipientList' => array (
				"Headline" => _('Allowed recipients'), 'attr' => 'kolabAllowSMTPRecipient',
				"Text" => _('Describes the allowed or disallowed SMTP recipient addresses for mail sent by this account (e.g. "domain.tld" or "-user@domain.tld").')
							. ' ' . _("Multiple values are separated by semicolon.")
			),
			'kolabAllowSMTPSender' => array (
				"Headline" => _('Allowed senders'), 'attr' => 'kolabAllowSMTPSender',
				"Text" => _('Describes the allowed or disallowed SMTP addresses sending mail to this account (e.g. "domain.tld" or "-user@domain.tld").')
			),
			'kolabAllowSMTPSenderList' => array (
				"Headline" => _('Allowed senders'), 'attr' => 'kolabAllowSMTPSender',
				"Text" => _('Describes the allowed or disallowed SMTP addresses sending mail to this account (e.g. "domain.tld" or "-user@domain.tld").')
							. ' ' . _("Multiple values are separated by semicolon.")
			),
			'mailQuota' => array(
				"Headline" => _('Mailbox quota'), 'attr' => 'mailQuota',
				"Text" => _('The maximum mailbox size in MB.')
			),
		);
		// upload fields
		$return['upload_columns'] = array(
			array(
				'name' => 'kolabUser_invPol',
				'description' => _('Invitation policy'),
				'help' => 'invPolList',
				'example' => 'user@domain:ACT_ALWAYS_ACCEPT,user2@domain:ACT_MANUAL'
			),
			array(
				'name' => 'kolabUser_mailQuota',
				'description' => _('Mailbox quota'),
				'help' => 'mailQuota',
				'example' => '2000',
			),
			array(
				'name' => 'kolabUser_aliases',
				'description' => _('Email aliases'),
				'help' => 'aliasList',
				'example' => 'user@domain,user2@domain'
			),
			array(
				'name' => 'kolabUser_delegates',
				'description' => _('Delegates'),
				'help' => 'delegateList',
				'example' => 'uid=user,ou=People,dc=example,dc=com;uid=user2,ou=People,dc=example,dc=com'
			),
			array(
				'name' => 'kolabUser_kolabAllowSMTPRecipient',
				'description' => _('Allowed recipients'),
				'help' => 'kolabAllowSMTPRecipientList',
				'example' => '.com; -.net',
			),
			array(
				'name' => 'kolabUser_kolabAllowSMTPSender',
				'description' => _('Allowed senders'),
				'help' => 'kolabAllowSMTPSenderList',
				'example' => '.com; -.net',
			),
		);
		// available PDF fields
		$return['PDF_fields'] = array(
			'invPol' => _('Invitation policy'),
			'mailQuota' => _('Mailbox quota'),
			'aliases' => _('Email aliases'),
			'delegate' => _('Delegates'),
			'kolabAllowSMTPRecipient' => _('Allowed recipients'),
			'kolabAllowSMTPSender' => _('Allowed senders'),
		);
		return $return;
	}

	/**
	* This function fills the error message array with messages
	*/
	function load_Messages() {
		$this->messages['invPol'][0] = array('ERROR', _('Target of invitation policy is invalid!'));  // third array value is set dynamically
		$this->messages['invPol'][1] = array('ERROR', _('Account %s:') . ' kolabUser_invPol', _('Policy list has invalid format!'));
		$this->messages['alias'][0] = array('ERROR', _('Email alias is invalid!'));  // third array value is set dynamically
		$this->messages['alias'][1] = array('ERROR', _('Account %s:') . ' kolabUser_aliases', _('Email alias list has invalid format!'));
		$this->messages['delegate'][0] = array('ERROR', _('Account %s:') . ' kolabUser_delegates', _('Unknown delegate address: %s'));
		$this->messages['kolabAllowSMTPRecipient'][0] = array('ERROR', _('Allowed recipients'), _('Please enter a valid recipient expression.'));
		$this->messages['kolabAllowSMTPRecipient'][1] = array('ERROR', _('Account %s:') . ' kolabUser_kolabAllowSMTPRecipient', _('Please enter a valid recipient expression.'));
		$this->messages['kolabAllowSMTPSender'][0] = array('ERROR', _('Allowed senders'), _('Please enter a valid sender expression.'));
		$this->messages['kolabAllowSMTPSender'][1] = array('ERROR', _('Account %s:') . ' kolabUser_kolabAllowSMTPSender', _('Please enter a valid sender expression.'));
		$this->messages['mailQuota'][0] = array('ERROR', _('Mailbox quota'), _('Please enter a number.'));
		$this->messages['mailQuota'][1] = array('ERROR', _('Account %s:') . ' kolabUser_mailQuota', _('Please enter a number.'));
	}

	/**
	 * Returns the HTML meta data for the main account page.
	 *
	 * @return htmlElement HTML meta data
	 */
	function display_html_attributes() {
		$return = new htmlTable();
		if (isset($this->attributes['objectClass']) && in_array('kolabInetOrgPerson', $this->attributes['objectClass'])) {
			$attrsI = $this->getAccountContainer()->getAccountModule('inetOrgPerson')->getAttributes();
			if ($this->getAccountContainer()->isNewAccount) {
				if ($this->getAccountContainer()->getAccountModule('posixAccount') != null) {
					$attrsP = $this->getAccountContainer()->getAccountModule('posixAccount')->getAttributes();
					if (!isset($attrsP['userpassword'][0]) || ($attrsP['userpassword'][0] == '')) {
						$message = new htmlStatusMessage('ERROR', _("Please enter a user password."));
						$message->colspan = 5;
						$return->addElement($message, true);
					}
				}
				elseif (!isset($attrsI['userpassword'][0]) || ($attrsI['userpassword'][0] == '')) {
					$message = new htmlStatusMessage('ERROR', _("Please enter a user password."));
					$message->colspan = 5;
					$return->addElement($message, true);
				}
			}
			if (!isset($attrsI['mail'][0])) {
				$message = new htmlStatusMessage('ERROR', _("Please enter an email address on this page: %s"), '', array($this->getAccountContainer()->getAccountModule('inetOrgPerson')->get_alias()));
				$message->colspan = 5;
				$return->addElement($message, true);
			}
			// mailbox quota
			$mailQuota = '';
			if (!empty($this->attributes['mailQuota'][0])) {
				$mailQuota = $this->attributes['mailQuota'][0] / 1024;
			}
			$mailQuotaField = new htmlTableExtendedInputField(_('Mailbox quota'), 'mailQuota', $mailQuota, 'mailQuota');
			$mailQuotaField->setMinimumAndMaximumNumber();
			$return->addElement($mailQuotaField, true);
			// invitation policies
			$return->addElement(new htmlSubTitle(_('Invitation policy')), true);
			$invitationContainer = new htmlTable();
			$invitationContainer->colspan = 3;
			// default invitation policy
			$defaultInvPol = $this->invitationPolicies['ACT_MANUAL'];
			if (isset($this->attributes['kolabInvitationPolicy'])) {
				for ($i = 0; $i < sizeof($this->attributes['kolabInvitationPolicy']); $i++) {
					$parts = explode(":", $this->attributes['kolabInvitationPolicy'][$i]);
					if (sizeof($parts) == 1) {
						$defaultInvPol = $this->invitationPolicies[$this->attributes['kolabInvitationPolicy'][$i]];
						unset($this->attributes['kolabInvitationPolicy'][$i]);
						$this->attributes['kolabInvitationPolicy'] = array_values($this->attributes['kolabInvitationPolicy']);
						break;
					}
				}
			}
			$invitationContainer->addElement(new htmlTableExtendedSelect('defaultInvPol', array_values($this->invitationPolicies), array($defaultInvPol), _('Anyone'), 'invPol'), true);
			// other invitation policies
			if (isset($this->attributes['kolabInvitationPolicy'])) {
				for ($i = 0; $i < sizeof($this->attributes['kolabInvitationPolicy']); $i++) {
					$parts = explode(":", $this->attributes['kolabInvitationPolicy'][$i]);
					if (sizeof($parts) == 2) {
						$invitationContainer->addElement(new htmlInputField('invPol1' . $i, $parts[0]));
						$invitationContainer->addElement(new htmlSelect('invPol2' . $i, array_values($this->invitationPolicies), array($this->invitationPolicies[$parts[1]])));
						$invitationContainer->addElement(new htmlButton('delInvPol' . $i, 'del.svg', true));
						$invitationContainer->addElement(new htmlHelpLink('invPol'), true);
					}
				}
			}
			// input box for new invitation policy
			$invitationContainer->addElement(new htmlInputField('invPol1', ''));
			$invitationContainer->addElement(new htmlSelect('invPol2', array_values($this->invitationPolicies)));
			$invitationContainer->addElement(new htmlButton('addInvPol', 'add.svg', true));
			$invitationContainer->addElement(new htmlHelpLink('invPol'), true);
			$return->addElement($invitationContainer, true);
			// mail aliases
			$return->addElement(new htmlSubTitle(_('Email aliases')), true);
			$this->addMultiValueInputTextField($return, 'alias', null);
			// delegates
			$delegatesData = searchLDAPByAttribute('mail', '*', 'inetOrgPerson', array('dn'), array('user'));
			$delegates = array();
			for ($i = 0; $i < sizeof($delegatesData); $i++) {
				$delegates[getAbstractDN($delegatesData[$i]['dn'])] = $delegatesData[$i]['dn'];
			}
			uksort($delegates, 'compareDN');
			$return->addElement(new htmlSubTitle(_('Delegates')), true);
			$delegatesContainer = new htmlTable();
			$delegatesContainer->colspan = 3;
			if (isset($this->attributes['kolabDelegate'])) {
				for ($i = 0; $i < sizeof($this->attributes['kolabDelegate']); $i++) {
					$delegateSelect = new htmlSelect('delegate' . $i, $delegates, array($this->attributes['kolabDelegate'][$i]));
					$delegateSelect->setSortElements(false);
					$delegateSelect->setHasDescriptiveElements(true);
					$delegatesContainer->addElement($delegateSelect);
					$delegatesContainer->addElement(new htmlButton('delDelegate' . $i, 'del.svg', true));
					if ($i == 0) {
						$delegatesContainer->addElement(new htmlHelpLink('delegate'));
					}
					$delegatesContainer->addNewLine();
				}
			}
			// input box for new delegate
			$delegatesSelectNew = new htmlSelect('delegate', $delegates);
			$delegatesSelectNew->setHasDescriptiveElements(true);
			$delegatesContainer->addElement($delegatesSelectNew);
			$delegatesContainer->addElement(new htmlButton('addDelegate', 'add.svg', true));
			if (empty($this->attributes['kolabDelegate'])) {
				$delegatesContainer->addElement(new htmlHelpLink('delegate'));
			}
			$return->addElement($delegatesContainer, true);
			$return->addElement(new htmlSubTitle(_('Options')), true);
			// allowed recipients
			$this->addMultiValueInputTextField($return, 'kolabAllowSMTPRecipient', _('Allowed recipients'));
			// allowed senders
			$this->addMultiValueInputTextField($return, 'kolabAllowSMTPSender', _('Allowed senders'));
		}
		else {
			$return->addElement(new htmlButton('addObjectClass', _('Add Kolab extension')));
		}
		return $return;
	}

	/**
	* Processes user input of the primary module page.
	* It checks if all input values are correct and updates the associated LDAP attributes.
	*
	* @return array list of info/error messages
	*/
	function process_attributes() {
		$errors = array();
		if (isset($_POST['addObjectClass'])) {
			$this->attributes['objectClass'][] = 'kolabInetOrgPerson';
			$this->attributes['objectClass'][] = 'mailrecipient';
		}
		else {
			if (isset($_POST['form_subpage_kolabUser_deleteUser_open']))  return array();
			$this->attributes['kolabInvitationPolicy'] = array();
			// check old invitation policies
			$policies = array_flip($this->invitationPolicies);
			$targets = array();
			$i = 0;
			while (isset($_POST['invPol1' . $i])) {
				if (isset($_POST['delInvPol' . $i])) {
					$i++;
					continue;
				}
				if (isset($_POST['invPol2' . $i]) && ($_POST['invPol1' . $i] != "") && !in_array($_POST['invPol1' . $i], $targets)) {
					$targets[] = $_POST['invPol1' . $i];
					// check invitation policy
					if (!get_preg($_POST['invPol1' . $i], 'email')) {
						$message = $this->messages['invPol'][0];
						$message[] = $_POST['invPol1' . $i];
						$errors[] = $message;
					}
					else {
						$this->attributes['kolabInvitationPolicy'][] = $_POST['invPol1' . $i] . ':' . $policies[$_POST['invPol2' . $i]];
					}
				}
				$i++;
			}
			// check new invitation policy
			if (isset($_POST['invPol1']) && ($_POST['invPol1'] != "") && !in_array($_POST['invPol1'], $targets)) {
				// check new invitation policy
				if (!get_preg($_POST['invPol1'], 'email')) {
					$message = $this->messages['invPol'][0];
					$message[] = $_POST['invPol1'];
					$errors[] = $message;
				}
				else {
					$this->attributes['kolabInvitationPolicy'][] = $_POST['invPol1'] . ':' . $policies[$_POST['invPol2']];
				}
			}
			// default invitation policy
			if (isset($_POST['defaultInvPol']) && ($_POST['defaultInvPol'] != "")) {
				$this->attributes['kolabInvitationPolicy'][] = $policies[$_POST['defaultInvPol']];
			}
			// mail aliases
			$this->processMultiValueInputTextField('alias', $errors, 'email');
			// check old delegates
			$this->attributes['kolabDelegate'] = array();
			$i = 0;
			while (isset($_POST['delegate' . $i])) {
				if (isset($_POST['delDelegate' . $i])) {
					$i++;
					continue;
				}
				$this->attributes['kolabDelegate'][] = $_POST['delegate' . $i];
				$i++;
			}
			// check new delegate
			if (isset($_POST['addDelegate']) && ($_POST['delegate'] != "")) {
				$this->attributes['kolabDelegate'][] = $_POST['delegate'];
			}
			$this->attributes['kolabDelegate'] = array_unique($this->attributes['kolabDelegate']);
			// allowed recipients
			$this->processMultiValueInputTextField('kolabAllowSMTPRecipient', $errors, 'kolabEmailPrefix');
			// allowed senders
			$this->processMultiValueInputTextField('kolabAllowSMTPSender', $errors, 'kolabEmailPrefix');
			// mailbox quota
			if (!empty($_POST['mailQuota']) && !get_preg($_POST['mailQuota'], 'digit')) {
				$errors[] = $this->messages['mailQuota'][0];
			}
			else {
				$mailQuota = array();
				if (!empty($_POST['mailQuota'])) {
					$mailQuota = array($_POST['mailQuota'] * 1024);
				}
				$this->attributes['mailQuota'] = $mailQuota;
			}
		}
		return $errors;
	}

	/**
	* This function returns true if all needed settings are done.
	*
	* @return true, if account can be saved
	*/
	function module_complete() {
		if (isset($this->attributes['objectClass']) && in_array('kolabInetOrgPerson', $this->attributes['objectClass'])) {
			$attrsI = $this->getAccountContainer()->getAccountModule('inetOrgPerson')->getAttributes();
			if (!$attrsI['mail'][0]) return false;
			if ($this->getAccountContainer()->isNewAccount) {
				if ($this->getAccountContainer()->getAccountModule('posixAccount') != null) {
					$attrsP = $this->getAccountContainer()->getAccountModule('posixAccount')->getAttributes();
					if (!$attrsP['userpassword'][0]) return false;
				}
				elseif (!$attrsI['userpassword'][0]) {
					return false;
				}
			}
		}
		return true;
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
	function save_attributes() {
		if (!in_array('kolabInetOrgPerson', $this->attributes['objectClass']) && !in_array('kolabInetOrgPerson', $this->orig['objectClass'])) {
			// skip saving if the extension was not added/modified
			return array();
		}
		return $this->getAccountContainer()->save_module_attributes($this->attributes, $this->orig);
	}

	/**
	* Loads the values of an account profile into internal variables.
	*
	* @param array $profile hash array with profile values (identifier => value)
	*/
	function load_profile($profile) {
		// profile mappings in meta data
		parent::load_profile($profile);
		// mailbox quota
		if (!empty($profile['kolab_mailQuota'][0])) {
			$this->attributes['mailQuota'][0] = $profile['kolab_mailQuota'][0] * 1024;
		}
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::build_uploadAccounts()
	 */
	function build_uploadAccounts($rawAccounts, $ids, &$partialAccounts, $selectedModules, &$type) {
		$messages = array();
		$delegates = searchLDAPByAttribute(null, null, 'inetOrgPerson', array('mail'), array('user'));
		for ($d = 0; $d < sizeof($delegates); $d++) {
			if (isset($delegates[$d]['dn'])) {
				$delegates[$d] = $delegates[$d]['dn'];
			}
		}
		for ($i = 0; $i < sizeof($rawAccounts); $i++) {
			// add object class
			if (!in_array("kolabInetOrgPerson", $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = "kolabInetOrgPerson";
			}
			if (!in_array("mailrecipient", $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = "mailrecipient";
			}
			// mailbox quota
			if (!empty($rawAccounts[$i][$ids['kolabUser_mailQuota']])) {
				if (get_preg($rawAccounts[$i][$ids['kolabUser_mailQuota']], 'digit')) {
					$partialAccounts[$i]['mailQuota'] = $rawAccounts[$i][$ids['kolabUser_mailQuota']] * 1024;
				}
				else {
					$errMsg = $this->messages['mailQuota'][1];
					array_push($errMsg, array($i));
					$messages[] = $errMsg;
				}
			}
			// add invitation policies
			if ($rawAccounts[$i][$ids['kolabUser_invPol']] != "") {
				$pols = explode(',', $rawAccounts[$i][$ids['kolabUser_invPol']]);
				// check format
				$policies = array_keys($this->invitationPolicies);
				$defaultFound = false;
				for ($p = 0; $p < sizeof($pols); $p++) {
					$parts = explode(":", $pols[$p]);
					// default policy
					if (sizeof($parts) == 1) {
						if (!$defaultFound && get_preg($parts[0], 'email')) {
							$partialAccounts[$i]['kolabInvitationPolicy'][] = $parts[0];
						}
						else {
							$errMsg = $this->messages['invPol'][1];
							array_push($errMsg, array($i));
							$messages[] = $errMsg;
						}
						$defaultFound = true;
					}
					// additional policies
					elseif (sizeof($parts) == 2) {
						if (get_preg($parts[0], 'email') && in_array($parts[1], $policies)) {
							$partialAccounts[$i]['kolabInvitationPolicy'][] = $pols[$p];
						}
						else {
							$errMsg = $this->messages['invPol'][1];
							array_push($errMsg, array($i));
							$messages[] = $errMsg;
						}
					}
					// invalid format
					else {
						$errMsg = $this->messages['invPol'][1];
						array_push($errMsg, array($i));
						$messages[] = $errMsg;
					}
				}
			}
			// add mail aliases
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabUser_aliases', 'alias', 'email', $this->messages['alias'][1], $messages, '/,[ ]*/');
			// add delegates
			if ($rawAccounts[$i][$ids['kolabUser_delegates']] != "") {
				$newDelegates = explode(';', $rawAccounts[$i][$ids['kolabUser_delegates']]);
				// check format
				for ($d = 0; $d < sizeof($newDelegates); $d++) {
					if (in_array($newDelegates[$d], $delegates)) {
						$partialAccounts[$i]['kolabDelegate'][] = $newDelegates[$d];
					}
					// invalid format
					else {
						$errMsg = $this->messages['delegate'][0];
						array_push($errMsg, array($i, $newDelegates[$d]));
						$messages[] = $errMsg;
					}
				}
			}
			// allowed recipients
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabUser_kolabAllowSMTPRecipient', 'kolabAllowSMTPRecipient', 'kolabEmailPrefix', $this->messages['kolabAllowSMTPRecipient'][1], $messages, '/;[ ]*/');
			// allowed senders
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabUser_kolabAllowSMTPSender', 'kolabAllowSMTPSender', 'kolabEmailPrefix', $this->messages['kolabAllowSMTPSender'][1], $messages, '/;[ ]*/');
		}
		return $messages;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfEntries()
	 */
	function get_pdfEntries($pdfKeys, $typeId) {
		$return = array();
		// invitation policies
		if (isset($this->attributes['kolabInvitationPolicy'][0])) {
			// find default policy
			$default = "";
			for ($i = 0; $i < sizeof($this->attributes['kolabInvitationPolicy']); $i++) {
				if (!strpos($this->attributes['kolabInvitationPolicy'][$i], ":")) {
					$default = $this->attributes['kolabInvitationPolicy'][$i];
					break;
				}
			}
			$pdfTable = new PDFTable(_('Invitation policy'));
			$pdfRow = new PDFTableRow();
			$pdfRow->cells[] = new PDFTableCell(_('Anyone') . ": " . $this->invitationPolicies[$default]);
			$pdfTable->rows[] = $pdfRow;
			for ($i = 0; $i < sizeof($this->attributes['kolabInvitationPolicy']); $i++) {
				$parts = explode(':', $this->attributes['kolabInvitationPolicy'][$i]);
				if (sizeof($parts) == 2) {
					$pdfRow = new PDFTableRow();
					$pdfRow->cells[] = new PDFTableCell($parts[0] . ": " . $this->invitationPolicies[$parts[1]]);
					$pdfTable->rows[] = $pdfRow;
				}
			}
			$this->addPDFTable($return, 'invPol', $pdfTable);
		}
		$this->addSimplePDFField($return, 'aliases', _('Email aliases'), 'alias');
		$this->addSimplePDFField($return, 'delegate', _('Delegates'), 'kolabDelegate', '; ');
		$this->addSimplePDFField($return, 'kolabAllowSMTPRecipient', _('Allowed recipients'));
		$this->addSimplePDFField($return, 'kolabAllowSMTPSender', _('Allowed senders'));
		$mailQuota = '';
		if (!empty($this->attributes['mailQuota'][0])) {
			$mailQuota = ($this->attributes['mailQuota'][0] / 1024) . 'MB';
		}
		$this->addPDFKeyValue($return, 'mailQuota', _('Mailbox quota'), $mailQuota);
		return $return;
	}

	/**
	 * Returns the meta HTML code for each input field.
	 * format: array(<field1> => array(<META HTML>), ...)
	 * It is not possible to display help links.
	 *
	 * @param array $fields list of active fields
	 * @param array $attributes attributes of LDAP account
	 * @param boolean $passwordChangeOnly indicates that the user is only allowed to change his password and no LDAP content is readable
	 * @param array $readOnlyFields list of read-only fields
	 * @return array list of meta HTML elements (field name => htmlResponsiveRow)
	 */
	function getSelfServiceOptions($fields, $attributes, $passwordChangeOnly, $readOnlyFields) {
		if ($passwordChangeOnly) {
			return array(); // no Kolab fields as long no LDAP content can be read
		}
		if (!in_array('kolabInetOrgPerson', $attributes['objectClass'])) {
			return array();
		}
		$return = array();
		// delegates
		if (in_array('kolabDelegate', $fields)) {
			$delegates = array();
			$sr = @ldap_search($_SESSION['ldapHandle']->getServer(), $this->selfServiceSettings->LDAPSuffix, '(&(objectClass=inetOrgPerson)(mail=*))', array('dn'), 0, 0, 0, LDAP_DEREF_NEVER);
			if ($sr) {
				$result = ldap_get_entries($_SESSION['ldapHandle']->getServer(), $sr);
				cleanLDAPResult($result);
				for ($i = 0; $i < sizeof($result); $i++) {
					$delegates[getAbstractDN($result[$i]['dn'])] = $result[$i]['dn'];
				}
			}
			$kolabDelegate = array();
			if (isset($attributes['kolabDelegate'])) {
				$kolabDelegate = $attributes['kolabDelegate'];
				// do not show existing delegation candidates in selection list
				for ($i = 0; $i < sizeof($kolabDelegate); $i++) {
					$key = array_search($kolabDelegate[$i], $delegates);
					if ($key !== false) {
						unset($delegates[$key]);
					}
				}
			}
			uksort($delegates, 'compareDN');
			$_SESSION['kolabUser_kolabDelegate'] = $kolabDelegate;
			$delegateContainer = new htmlTable();
			for ($i = 0; $i < sizeof($kolabDelegate); $i++) {
				$delegateContainer->addElement(new htmlOutputText($kolabDelegate[$i]));
				if (!in_array('kolabDelegate', $readOnlyFields)) {
					$delegateContainer->addElement(new htmlTableExtendedInputCheckbox('delDelegate_' . $i, false, _('Delete'), null, false), true);
				}
				else {
					$delegateContainer->addNewLine();
				}
			}
			if (!in_array('kolabDelegate', $readOnlyFields)) {
				$delegateSelect = new htmlSelect('new_delegate_value', $delegates);
				$delegateSelect->setSortElements(false);
				$delegateSelect->setHasDescriptiveElements(true);
				$delegateContainer->addElement($delegateSelect);
				$delegateContainer->addElement(new htmlTableExtendedInputCheckbox('new_delegate', false, _("Add"), null, false), true);
			}
			$delegateLabel = new htmlOutputText($this->getSelfServiceLabel('kolabDelegate', _('Delegates')));
			$row = new htmlResponsiveRow();
			$row->addLabel($delegateLabel);
			$row->addField($delegateContainer);
			$return['kolabDelegate'] = $row;
		}
		// invitation policies
		if (in_array('kolabInvitationPolicy', $fields)) {
			$invitationContainer = new htmlTable();
			// default invitation policy
			$defaultInvPol = $this->invitationPolicies['ACT_MANUAL'];
			for ($i = 0; $i < sizeof($attributes['kolabInvitationPolicy']); $i++) {
				$parts = explode(":", $attributes['kolabInvitationPolicy'][$i]);
				if (sizeof($parts) == 1) {
					$defaultInvPol = $this->invitationPolicies[$attributes['kolabInvitationPolicy'][$i]];
					unset($attributes['kolabInvitationPolicy'][$i]);
					$attributes['kolabInvitationPolicy'] = array_values($attributes['kolabInvitationPolicy']);
					break;
				}
			}
			if (!in_array('kolabDelegate', $readOnlyFields)) {
				$invitationContainer->addElement(new htmlTableExtendedSelect('defaultInvPol', array_values($this->invitationPolicies), array($defaultInvPol), _('Anyone')), true);
			}
			else {
				$invitationContainer->addElement(new htmlOutputText(_('Anyone')));
				$invitationContainer->addElement(new htmlOutputText($defaultInvPol), true);
			}
			// other invitation policies
			for ($i = 0; $i < sizeof($attributes['kolabInvitationPolicy']); $i++) {
				$parts = explode(":", $attributes['kolabInvitationPolicy'][$i]);
				if (sizeof($parts) == 2) {
					if (!in_array('kolabDelegate', $readOnlyFields)) {
						$invitationContainer->addElement(new htmlInputField('invPol1' . $i, $parts[0]));
						$invitationContainer->addElement(new htmlSelect('invPol2' . $i, array_values($this->invitationPolicies), array($this->invitationPolicies[$parts[1]])));
						$invitationContainer->addElement(new htmlTableExtendedInputCheckbox('delInvPol' . $i, false, _("Remove"), null, false), true);
					}
					else {
						$invitationContainer->addElement(new htmlOutputText($parts[0]));
						$invitationContainer->addElement(new htmlOutputText($this->invitationPolicies[$parts[1]]), true);
					}
				}
			}
			// input box for new invitation policy
			if (!in_array('kolabDelegate', $readOnlyFields)) {
				$invitationContainer->addElement(new htmlInputField('invPol1', ''));
				$invitationContainer->addElement(new htmlSelect('invPol2', array_values($this->invitationPolicies)));
				$invitationContainer->addElement(new htmlTableExtendedInputCheckbox('addInvPol', false, _("Add"), null, false), true);
			}
			$invitationLabel = new htmlOutputText($this->getSelfServiceLabel('kolabInvitationPolicy', _('Invitation policy')));
			$row = new htmlResponsiveRow();
			$row->addLabel($invitationLabel);
			$row->addField($invitationContainer);
			$return['kolabInvitationPolicy'] = $row;
		}
		return $return;
	}

	/**
	 * Checks if all input values are correct and returns the LDAP attributes which should be changed.
	 * <br>Return values:
	 * <br>messages: array of parameters to create status messages
	 * <br>add: array of attributes to add
	 * <br>del: array of attributes to remove
	 * <br>mod: array of attributes to modify
	 * <br>info: array of values with informational value (e.g. to be used later by pre/postModify actions)
	 *
	 * Calling this method does not require the existence of an enclosing {@link accountContainer}.
	 *
	 * @param string $fields input fields
	 * @param array $attributes LDAP attributes
	 * @param boolean $passwordChangeOnly indicates that the user is only allowed to change his password and no LDAP content is readable
	 * @param array $readOnlyFields list of read-only fields
	 * @return array messages and attributes (array('messages' => array(), 'add' => array('mail' => array('test@test.com')), 'del' => array(), 'mod' => array(), 'info' => array()))
	 */
	function checkSelfServiceOptions($fields, $attributes, $passwordChangeOnly, $readOnlyFields) {
		$return = array('messages' => array(), 'add' => array(), 'del' => array(), 'mod' => array(), 'info' => array());
		if ($passwordChangeOnly) {
			return $return; // skip processing if only a password change is done
		}
		if (!in_array_ignore_case('kolabInetOrgPerson', $attributes['objectClass'])) {
			return $return;
		}
		$attributeNames = array(); // list of attributes which should be checked for modification
		$attributesNew = $attributes;
		// delegates
		if (in_array('kolabDelegate', $fields) && !in_array('kolabDelegate', $readOnlyFields)) {
			$attributeNames[] = 'kolabDelegate';
			// new delegation
			if (isset($_POST['new_delegate']) && ($_POST['new_delegate'] == 'on')) {
				$attributesNew['kolabDelegate'][] = $_POST['new_delegate_value'];
			}
			// check for deleted delegations
			$postKeys = array_keys($_POST);
			for ($i = 0; $i < sizeof($postKeys); $i++) {
				if (strpos($postKeys[$i], 'delDelegate_') !== false) {
					$sKey = substr($postKeys[$i], strlen('delDelegate_'));
					$key = array_search($_SESSION['kolabUser_kolabDelegate'][$sKey], $attributesNew['kolabDelegate']);
					if ($key !== false) {
						unset($attributesNew['kolabDelegate'][$key]);
						$attributesNew['kolabDelegate'] = array_values($attributesNew['kolabDelegate']);
					}
				}
			}
		}
		// invitation policies
		if (in_array('kolabInvitationPolicy', $fields) && !in_array('kolabInvitationPolicy', $readOnlyFields)) {
			$attributeNames[] = 'kolabInvitationPolicy';
			$policies = array_flip($this->invitationPolicies);
			$attributesNew['kolabInvitationPolicy'] = array();
			// check old invitation policies
			$targets = array();
			$i = 0;
			while (isset($_POST['invPol1' . $i])) {
				if (isset($_POST['delInvPol' . $i])) {
					$i++;
					continue;
				}
				if (isset($_POST['invPol2' . $i]) && ($_POST['invPol1' . $i] != "") && !in_array($_POST['invPol1' . $i], $targets)) {
					$targets[] = $_POST['invPol1' . $i];
					// check invitation policy
					if (!get_preg($_POST['invPol1' . $i], 'email')) {
						$message = $this->messages['invPol'][0];
						$message[] = $_POST['invPol1' . $i];
						$errors[] = $message;
					}
					else {
						$attributesNew['kolabInvitationPolicy'][] = $_POST['invPol1' . $i] . ':' . $policies[$_POST['invPol2' . $i]];
					}
				}
				$i++;
			}
			// check new invitation policy
			if (isset($_POST['invPol1']) && ($_POST['invPol1'] != "") && !in_array($_POST['invPol1'], $targets)) {
				// check new invitation policy
				if (!get_preg($_POST['invPol1'], 'email')) {
					$message = $this->messages['invPol'][0];
					$message[] = $_POST['invPol1'];
					$errors[] = $message;
				}
				else {
					$attributesNew['kolabInvitationPolicy'][] = $_POST['invPol1'] . ':' . $policies[$_POST['invPol2']];
				}
			}
			// default invitation policy
			if (isset($_POST['defaultInvPol']) && ($_POST['defaultInvPol'] != "")) {
				$attributesNew['kolabInvitationPolicy'][] = $policies[$_POST['defaultInvPol']];
			}
		}
		// find differences
		for ($i = 0; $i < sizeof($attributeNames); $i++) {
			$attrName = $attributeNames[$i];
			if (isset($attributes[$attrName]) && !isset($attributesNew[$attrName])) {
				$return['del'][$attrName] = $attributes[$attrName];
			}
			elseif (!isset($attributes[$attrName]) && isset($attributesNew[$attrName])) {
				$return['add'][$attrName] = $attributesNew[$attrName];
			}
			else {
				if (isset($attributes[$attrName])) {
					for ($a = 0; $a < sizeof($attributes[$attrName]); $a++) {
						if (!in_array($attributes[$attrName][$a], $attributesNew[$attrName])) {
							$return['mod'][$attrName] = $attributesNew[$attrName];
							break;
						}
					}
				}
				if (isset($attributesNew[$attrName])) {
					for ($a = 0; $a < sizeof($attributesNew[$attrName]); $a++) {
						if (!in_array($attributesNew[$attrName][$a], $attributes[$attrName])) {
							$return['mod'][$attrName] = $attributesNew[$attrName];
							break;
						}
					}
				}
			}
		}
		return $return;
	}

}


?>
