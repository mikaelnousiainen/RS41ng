

var shiftWindow = function() {
    scrollBy(0, -150);
};

if (location.hash) shiftWindow();
window.addEventListener("hashchange", shiftWindow);

function ChangeUrl(newUrl)
{
    document.location.href = newUrl;
    InspectUrlForField();
    shiftWindow();
}

function InspectUrlForField()
{

    var href = document.location.hash.substring(1);
    if (href.indexOf('_field_') > -1)
    {
        for (var a_els = document.getElementsByClassName("dark-ref numbers"), i = a_els.length; i--;)
        {
            if (a_els[i].tagName.toLowerCase() == "a")
            {
                if(a_els[i].name == href)
                {
                    var field_div = a_els[i].parentNode;
                    var panel_body = field_div.parentNode;
                    var panel_collapse = panel_body.parentNode;
                    /* Remove the 'panel_collapse_details_' (23 chars) to get ps id.
                     * Remove the 'field_' (6 chars) to get the field id.
                     */
                    ExpandFieldPanel(panel_collapse.id.substring(23), a_els[i].id.substring(6));
                }
            }
        }
    }
}

function GlobalExpandAll() {
    global_expand_all_tables();
    GlobalExpandFieldHeadings();
    GlobalExpandFieldDetails();
}

function GlobalExpandFieldHeadings() {
    for (var p_els = document.getElementsByClassName("panel-collapse collapse"), i = p_els.length; i--;) {
        if (p_els[i].tagName.toLowerCase() == "div") {
            /* Add 'in' from the class and remove any style = "height: 0px;" settings. */
            if (!(p_els[i].classList.contains("in"))) {
                p_els[i].classList.add("in")
            }
            p_els[i].removeAttribute("style");

            /* Update H6 anchor class for collapsed. */
            a_field_heading = p_els[i].parentNode.getElementsByTagName("div")[0].getElementsByTagName("a")[0];
            if (a_field_heading.classList.contains("collapsed")) {
                a_field_heading.classList.remove("collapsed")
            }
        }
    }
}

function GlobalExpandFieldDetails() {
    for (var p_els = document.getElementsByClassName("panel-collapse collapse"), i = p_els.length; i--;) {
        if (p_els[i].tagName.toLowerCase() == "div") {
            /* Add 'in' from the class and remove any style = "height: 0px;" settings. */
            if (!(p_els[i].classList.contains("in"))) {
                p_els[i].classList.add("in")
            }
            p_els[i].removeAttribute("style");

            /* Update H6 anchor class for collapsed. */
            a_field_heading = p_els[i].parentNode.getElementsByTagName("div")[0].getElementsByTagName("a")[0];
            if (a_field_heading.classList.contains("collapsed")) {
                a_field_heading.classList.remove("collapsed");
            }

            /* Panel collapse details. */
            /* Add 'in' from the class and remove any style = "height: 0px;" settings. */
            for (var f_els = p_els[i].getElementsByClassName("panel-body")[0].getElementsByClassName("collapse"), j = f_els.length; j--;) {
                if (!(f_els[j].classList.contains("in")))
                {
                    f_els[j].classList.add("in");
                }
                f_els[j].removeAttribute("style");
            }

            for (var a_els = p_els[i].getElementsByClassName("dark-ref numbers"), k = f_els.length; k--;) {
                if (!(a_els[k].classList.contains("collapsed")))
                {
                    f_els[k].classList.remove("collapsed");
                }
            }



        }
    }
}


function GlobalCollapseAll() {
    global_collapse_all_tables();
    GlobalCollapseFieldDetails();
    GlobalCollapseFieldHeadings();
}

function GlobalCollapseFieldHeadings() {
    for (var p_els = document.getElementsByClassName("panel-collapse collapse"), i = p_els.length; i--;) {
         /* Remove 'in' from the class and set style = "height: 0px;" settings. */
        if (p_els[i].tagName.toLowerCase() == "div") {
            if (p_els[i].classList.contains("in")) {
                p_els[i].classList.remove("in");
            }
            p_els[i].setAttribute("style", "height: 0px;");
            /* Update H6 anchor class for collapsed. */
            a_field_heading = p_els[i].parentNode.getElementsByTagName("div")[0].getElementsByTagName("a")[0];
            a_field_heading.classList.add("collapsed");
        }
    }
}

function GlobalCollapseFieldDetails() {
    var i = 0;
    var j = 0;
    var p_els = [];

    /* Updated the div class */
    for (p_els = document.getElementsByClassName("panel-collapse collapse"), i = p_els.length; i--;) {
        if (p_els[i].tagName.toLowerCase() == "div") {
            var f_els = [];
            for (f_els = p_els[i].getElementsByClassName("panel-body")[0].getElementsByClassName("collapse"), j = f_els.length; j--;) {
                if (f_els[j].classList.contains("in")) {
                    f_els[j].classList.remove("in");
                }
            }

        }
    }
    /* Updated the anchor class */
    for (p_els = document.getElementsByClassName("panel-title field-panel-title"), i = p_els.length; i--;) {
        var a_els = [];
        for (a_els = p_els[i].getElementsByTagName("a"), j = a_els.length; j--;) {
            if ((a_els[j].classList.contains("dark-ref")) && (a_els[j].classList.contains("numbers"))) {
                if (!(a_els[j].classList.contains("collapsed"))) {
                    a_els[j].classList.add("collapsed");
                }
            }
        }
    }
}


