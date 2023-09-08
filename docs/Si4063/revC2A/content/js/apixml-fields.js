
function ExpandFieldPanel(ps_id, f_id)
{
    var psPanelId = 'panel_collapse_details_' + ps_id;
    var divFieldPanelId  = 'div_field_' + f_id;

    var psPanel = document.getElementById(psPanelId);
    var divFieldPanel = document.getElementById(divFieldPanelId);

    /* Add 'in' from the class and remove any style = "height: 0px;" settings. */
    if (!(psPanel.classList.contains("in"))) {
        psPanel.classList.add("in");
    }
    psPanel.removeAttribute("style");

    /* Update H6 anchor class for collapsed. */
    a_field_heading = psPanel.parentNode.getElementsByTagName("div")[0].getElementsByTagName("a")[0];
    if (a_field_heading.classList.contains("collapsed")) {
        a_field_heading.classList.remove("collapsed");
    }
    /* Add 'in' from the class and remove any style = "height: 0px;" settings. */
    if (!(divFieldPanel.classList.contains("in"))) {
        divFieldPanel.classList.add("in");
    }
    divFieldPanel.removeAttribute("style");

    /* Clear any 'collapsed' in the class list for field detail name anchor. */
    for (var a_field_details = divFieldPanel.parentNode.getElementsByTagName("a"), i = a_field_details.length; i--;) {
        dt_attr = a_field_details[i].getAttribute("data-target");
        if ((dt_attr) && (dt_attr.substring(1) == divFieldPanelId)) {
            if (a_field_details[i].classList.contains("collapsed")) {
                a_field_details[i].classList.remove("collapsed");
            }
        }
    }
}

