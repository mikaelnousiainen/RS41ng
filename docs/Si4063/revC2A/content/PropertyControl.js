
    function ShowPropertyActivation(selection)
    {
        // hide all property activation div
        var i;
        var div_id;
        for (i=0 ; i < selection.length; i++)
        {
            div_id = selection.options[i].value;
            document.getElementById(div_id).style.display = "none";
        } 
        // show selected index
        div_id = selection.options[selection.selectedIndex].value;
        document.getElementById(div_id).style.display = "";
    }

    function ExpandPropertyFields(ps_id_suffix)
    {
        // get parameter set id suffix from title attribute
        CollapsibleLists.expandList(getItem('field_details_' + ps_id_suffix)); 
        toggleItems('expand_all_fields_prop_' + ps_id_suffix, 
                    'collapse_all_fields_prop_' + ps_id_suffix);
    }
    
    function CollapsePropertyFields(ps_id_suffix)
    {
        CollapsibleLists.collapseList(getItem('field_details_' + ps_id_suffix)); 
        toggleItems('expand_all_fields_prop_' + ps_id_suffix,
                    'collapse_all_fields_prop_' + ps_id_suffix);
    }
   
   
    function ExpandPropertyField(ps_id_suffix, field_id_suffix) 
    {
        // expand field details
        field_item_li = getItem('field_' + field_id_suffix);
        CollapsibleLists.expandListItem(field_item_li); 
        
        // show collapse button image on property field details
        // hide plus button, show minus button
        getItem('expand_all_fields_prop_' + ps_id_suffix).style.display = "none";
        getItem('collapse_all_fields_prop_' + ps_id_suffix).style.display = "";
    }
    
    
    function ExpandPropertyFieldEnum(ps_id_suffix, field_id_suffix) 
    {
        // expand field details
        field_item_li = getItem('field_' + field_id_suffix);
        CollapsibleLists.expandListItem(field_item_li); 
        // expand enum
        CollapsibleLists.expandListItem(getItem('field_' + field_id_suffix + '_enum'));
        CollapsibleLists.expandListItem(getItem('field_' + field_id_suffix + '_enum_ul'));

        // show collapse button image on property field details
        // hide plus button, show minus button
        getItem('expand_all_fields_prop_' + ps_id_suffix).style.display = "none";
        getItem('collapse_all_fields_prop_' + ps_id_suffix).style.display = "";
    }
