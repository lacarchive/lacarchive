The Entity reference autofill module gives Entity reference autocomplete
fields support to populate fields in their form with values from the
referenced entities.


Installation
------------

This module is dependant on the Entity reference module,
which can be downloaded from https://drupal.org/node/2140229


Configuration
-------------

- First, you need to add a field you want to be auto-filled to your entity.

- If you plan to load the referenced values from another entity type or bundle,
  you will need to add the same to the referenced entity.

- When you have set up both a source and destination field, add an
  entityreference field to the source entity. Choose the "Autocomplete" widget,
  as this is the only widget currently supported by the module.
  
- Now, in the reference field's instance settings, there should be a
  new fieldset, "Autofill settings". Enable autofill and select the fields
  you want to be autofilled.
  
- Go to the entity form and try it out by loading a value into the
  reference field.
