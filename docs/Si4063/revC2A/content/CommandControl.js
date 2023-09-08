
    function ExpandCommandFields(ps_id_suffix)
    {
        CollapsibleLists.expandList(getItem('field_details_' + ps_id_suffix));
        toggleItems('expand_all_fields_cmd_' + ps_id_suffix, 
                    'collapse_all_fields_cmd_' + ps_id_suffix);   
    }   
   
    function CollapseCommandFields(ps_id_suffix)
    {
        CollapsibleLists.collapseList(getItem('field_details_' + ps_id_suffix));
        toggleItems('expand_all_fields_cmd_' + ps_id_suffix,
                    'collapse_all_fields_cmd_' + ps_id_suffix);   
    }   
   
   
    
    function ExpandCommandField(ps_id_suffix, field_id_suffix) 
    {
        // expand field details
        field_item_li = getItem('field_' + field_id_suffix);
        CollapsibleLists.expandListItem(field_item_li);
        
        // show collapse button image on command field details
        // hide plus button, show minus button
        getItem('expand_all_fields_cmd_' + ps_id_suffix).style.display = "none";
        getItem('collapse_all_fields_cmd_' + ps_id_suffix).style.display = "";
    
    }
    
    
    function ExpandCommandFieldEnum(ps_id_suffix, field_id_suffix)
    {
        // expand field details
        field_item_li = getItem('field_' + field_id_suffix);
        CollapsibleLists.expandListItem(field_item_li);
        // expand enum
        CollapsibleLists.expandListItem(getItem('field_' + field_id_suffix + '_enum'));
        CollapsibleLists.expandListItem(getItem('field_' + field_id_suffix + '_enum_ul'));

        // show collapse button image on command field details
        // hide plus button, show minus button
        getItem('expand_all_fields_cmd_' + ps_id_suffix).style.display = "none";
        getItem('collapse_all_fields_cmd_' + ps_id_suffix).style.display = "";
    }
