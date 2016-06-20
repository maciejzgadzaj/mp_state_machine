<?php

/**
 * @file
 * Views field handler integration for the entity state properties.
 */

/**
 * Field handler to show the entity state properties.
 */
class mp_state_machine_handler_field_state extends views_handler_field {
  /**
   * {@inheritdoc}
   */
  public function construct() {
    parent::construct();

    // Set real_field in order to make it generate a field_alias.
    $this->real_field = 'order_id';
  }


  function render($values) {
    $entity_type = str_replace('_revision', '', $this->table);
    $state_type = $this->field;

    $entity_info = entity_get_info($entity_type);
    $revision_key = $entity_info['entity keys']['revision'];

    $entity = entity_revision_load($entity_type, $values->$revision_key);

    $state_machine = mp_state_machine_load($entity_type, $entity, $state_type);
    return $state_machine->get_current_state();
  }
}
