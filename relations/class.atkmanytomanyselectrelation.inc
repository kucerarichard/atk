<?php
/**
 * This file is part of the Achievo ATK distribution.
 * Detailed copyright and licensing information can be found
 * in the doc/COPYRIGHT and doc/LICENSE files which should be
 * included in the distribution.
 *
 * @package atk
 * @subpackage relations
 *
 * @copyright (c) 2000-2007 Ivo Jansch
 * @license http://www.achievo.org/atk/licensing ATK Open Source License
 *
 * @version $Revision: 5798 $
 * $Id: class.atkmanytomanylistrelation.inc 5798 2008-03-05 13:41:26Z guido $
 */

userelation("atkmanytomanyrelation");
userelation('atkmanytoonerelation');

define('AF_MANYTOMANYSELECT_DETAILEDIT', AF_SPECIFIC_1);

/**
 * Many-to-many select relation.
 *
 * The relation shows allows you to add one record at a time to a 
 * many-to-many relation using auto-completion or a select page.
 *
 * @author Peter C. Verhage <peter@achievo.org>
 * @package atk
 * @subpackage relations
 */
class atkManyToManySelectRelation extends atkManyToManyRelation
{
  const SEARCH_MODE_EXACT      = atkManyToOneRelation::SEARCH_MODE_EXACT;
  const SEARCH_MODE_STARTSWITH = atkManyToOneRelation::SEARCH_MODE_STARTSWITH;
  const SEARCH_MODE_CONTAINS   = atkManyToOneRelation::SEARCH_MODE_CONTAINS;
  
  /**
   * The many-to-one relation.
   * 
   * @var atkManyToOneRelation
   */
  private $m_manyToOneRelation = null;
  
  /**
   * Constructs a new many-to-many select relation.
   * 
   * @param string $name        The name of the relation
   * @param string $link         The full name of the node that is used as
   *                            intermediairy node. The intermediairy node is
   *                            assumed to have 2 attributes that are named
   *                            after the nodes at both ends of the relation.
   *                            For example, if node 'project' has a M2M relation
   *                            with 'activity', then the intermediairy node
   *                            'project_activity' is assumed to have an attribute
   *                            named 'project' and one that is named 'activity'.
   *                            You can set your own keys by calling setLocalKey()
   *                            and setRemoteKey()
   * @param string $destination The full name of the node that is the other
   *                            end of the relation.
   * @param int $flags          Flags for the relation.
   */
  public function __construct($name, $link, $destination, $flags=0)
  {
    parent::__construct($name, $link, $destination, $flags);
    
    $relation = new atkManyToOneRelation($this->fieldName().'_m2msr_add', $this->m_destination, AF_MANYTOONE_AUTOCOMPLETE|AF_HIDE);
    $relation->setDisabledModes(DISABLED_VIEW|DISABLED_EDIT);
    $relation->setLoadType(NOLOAD);
    $relation->setStorageType(NOSTORE);
    $this->m_manyToOneRelation = $relation;      
  }
  
  /**
   * Initialize.
   */
  public function init()
  {
    $this->getOwnerInstance()->add($this->getManyToOneRelation());    
  }
  
  /**
   * Return the many-to-one relation we will use for the selection
   * of new records etc.
   * 
   * @return atkManyToOneRelation
   */
  protected function getManyToOneRelation()
  {
    return $this->m_manyToOneRelation;
  }
  
  /**
   * Return a piece of html code to edit the attribute.
   * 
   * @param $record current record
   * 
   * @return string piece of html code
   */
  public function edit($record, $fieldprefix="", $mode="")
  {
    $this->createDestination();
    $this->createLink();
    
    $this->getOwnerInstance()->getPage()->register_script(atkconfig('atkroot').'atk/javascript/class.'.strtolower(__CLASS__).'.js');
    $this->getOwnerInstance()->addStyle('atkmanytomanyselectrelation.css');
    
    $selectedKeys = $this->getSelectedRecords($record);
    
    // check for record returned from select action
    $returnKey = $this->getManyToOneRelation()->getHtmlId($fieldprefix);
    if (isset($this->getOwnerInstance()->m_postvars[$returnKey]) && 
        !empty($this->getOwnerInstance()->m_postvars[$returnKey]))
    {
      $selectedKeys[] = $this->getOwnerInstance()->m_postvars[$returnKey];    
    }
    
    $selectedRecords = array();
    if (count($selectedKeys) > 0)
    {
      $selector = '('.implode(') OR (', $selectedKeys).')';
      $selectedRecords = $this->getDestination()->selectDb($selector);
    }
    
    $id = $this->getHtmlId($fieldprefix);
    
    $result = 
      '<div class="atkmanytomanyselectrelation">
         <ul id="'.$id.'_selection" class="atkmanytomanyselectrelation-selection">';
    
    foreach ($selectedRecords as $selectedRecord)
    {
      $result .= $this->renderSelectedRecord($selectedRecord, $fieldprefx);
    }

    $result .= $this->renderAdditionField($fieldprefix, $mode);
    
    $result .= '
        </ul>
      </div>';
    
    return $result;
  }
  
  /**
   * Render selected record.
   *
   * @param array  $record      selected record
   * @param string $fieldprefix field prefix
   */
  protected function renderSelectedRecord($record, $fieldprefix)
  {
    $name = $this->getHtmlId($fieldprefix).'[]['.$this->getRemoteKey().']';
    $key = $record[$this->getDestination()->primaryKeyField()];
    
    $actions = array();
    
    if ($this->hasFlag(AF_MANYTOMANYSELECT_DETAILEDIT) && $this->getDestination()->allowed('edit', $record))
    {
      $actions[] = href(dispatch_url($this->getDestination()->atkNodeType(), 'edit', array('atkselector' => $this->getDestination()->primaryKey($record))), $this->text('edit'), SESSION_NESTED, true);
    }

    $actions[] = '<a href="javascript:void(0)" onclick="ATK.ManyToManySelectRelation.delete(this); return false;">'.$this->text('delete').'</a>';    
    
    $result = '
      <li class="atkmanytomanyselectrelation-selected">
        <input type="hidden" name="'.$name.'" value="'.htmlentities($key).'"/>
        <span>'.htmlentities($this->getDestination()->descriptor($record)).'</span>
        ('.implode(' / ', $actions).')
      </li>
    ';

    return $result;   
  }
  
  /**
   * Render addition field.
   * 
   * @param string $fieldprefix field prefix
   */
  protected function renderAdditionField($fieldprefix, $mode)
  {
    $url = partial_url($this->getOwnerInstance()->atkNodeType(), $mode, 'attribute.'.$this->fieldName().'.selectedrecord', array('fieldprefix' => $fieldprefix));
    
    $relation = $this->getManyToOneRelation();
    $relation->addOnChangeHandler("
      ATK.ManyToManySelectRelation.add(el, '{$url}');
    ");
    
    return '
      <li class="atkmanytomanyselectrelation-addition">
        '.$relation->edit(array(), $fieldprefix, $mode).'
      </li>';
  }
  
  /**
   * Partial selected record.
   */
  public function partial_selectedrecord()
  {
    $this->createDestination();
    $this->createLink();    
    
    $fieldprefix = $this->getOwnerInstance()->m_postvars['fieldprefix'];
    $selector = $this->getOwnerInstance()->m_postvars['selector'];
    
    if (empty($selector)) return '';
    
    $record = $this->getDestination()->select($selector)->firstRow();    
    
    return $this->renderSelectedRecord($record, $fieldprefix);    
  }
  
  /**
   * Set the searchfields for the autocompletion. By default the
   * descriptor fields are used.
   *
   * @param array $searchFields
   */
  public function setAutoCompleteSearchFields($searchFields)
  {
    $this->getManyToOneRelation()->setAutoCompleteSearchFields($searchFields);
  }

  /**
   * Set the searchmode for the autocompletion:
   * exact, startswith(default) or contains.
   *
   * @param array $searchmode
   */
  public function setAutoCompleteSearchMode($mode)
  {
    $this->getManyToOneRelation()->setAutoCompleteSearchMode($mode);
  }

  /**
   * Set the case-sensitivity for the autocompletion search (true or false).
   *
   * @param array $caseSensitive
   */
  public function setAutoCompleteCaseSensitive($caseSensitive)
  {
    $this->getManyToOneRelation()->setAutoCaseSensitive($caseSensitive);
  }

  /**
   * Sets the minimum number of characters before auto-completion kicks in.
   *
   * @param int $chars
   */
  public function setAutoCompleteMinChars($chars)
  {
    $this->getManyToOneRelation()->setAutoCompleteMinChars($chars);
  }
}