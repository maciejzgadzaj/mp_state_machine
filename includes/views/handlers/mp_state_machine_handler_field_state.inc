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


  /**
   * Specifies the options this handler uses.
   */
  public function option_definition() {
    $options = parent::option_definition();
    $options['format_name'] = array('default' => TRUE);
    return $options;
  }

  /**
   * Returns an option form for setting this handler's options.
   */
  public function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['format_name'] = array(
      '#title' => t('Use human-readable name'),
      '#type' => 'checkbox',
      '#description' => t("If this is checked, the values' names will be displayed instead of their internal identifiers."),
      '#default_value' => $this->options['format_name'],
      '#weight' => -5,
    );
  }

  /**
   * Renders field values.
   */
  function render($values) {
    $entity_type = str_replace('_revision', '', $this->table);
    $state_type = $this->field;

    $entity_info = entity_get_info($entity_type);
    $revision_key = $entity_info['entity keys']['revision'];

    $entity = entity_revision_load($entity_type, $values->$revision_key);

    $state_machine = mp_state_machine_load($entity_type, $entity, $state_type);
    $current_state = $state_machine->get_current_state();

    // If 'Use human-readable name' option is enabled.
    if ($this->options['format_name']) {
      $states = mp_state_machine_states();
      if (!empty($states[$state_type][$current_state])) {
        $current_state = $states[$state_type][$current_state]['label'];
      }
    }

    return $current_state;
  }
}
