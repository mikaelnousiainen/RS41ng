/* dbg.js
 * Debug output logging class
 *
 * Copyright Devan Lai 2017
 *
 * Ported from lib/dbg.py in the pystlink project,
 * Copyright Pavel Revak 2015
 *
 */

/* eslint no-console: "off" */

export default class Dbg {
    constructor(verbose, log = null) {
        this._verbose = verbose;
        this._bargraph_msg = null;
        this._bargraph_min = null;
        this._bargraph_max = null;
        this._progress_bar = null;
        this._start_time = null;
        this._log = log;
    }

    _msg(msg, level, cls = null) {
        if ((this._verbose >= level)) {
            if (cls == "error") {
                console.error(msg);
            } else if (cls == "warning") {
                console.warn(msg);
            } else if (cls == "debug") {
                console.debug(msg);
            } else if (cls == "info") {
                console.info(msg);
            } else {
                console.log(msg);
            }
            if (this._log !== null) {
                let info = document.createElement("div");
                info.textContent = msg;
                if (cls !== null) {
                    info.className = cls;
                }
                this._log.appendChild(info);
            }
        }
    }

    clear() {
        if (this._log) {
            this._log.innerHTML = "";
        }
        if (this._progress_bar) {
            this._progress_bar = null;
        }
    }

    debug(msg, level = 3) {
        this._msg(msg, level, "debug");
    }

    verbose(msg, level = 2) {
        this._msg(msg, level, "verbose");
    }

    info(msg, level = 1) {
        this._msg(msg, level, "info");
    }

    message(msg, level = 0) {
        this._msg(msg, level, "message");
    }

    error(msg, level = 0) {
        this._msg(msg, level, "error");
    }

    warning(msg, level = 0) {
        this._msg(msg, level, "warning");
    }

    bargraph_start(msg, value_min = 0, value_max = 100, level = 1) {
        this._start_time = Date.now();
        if ((this._verbose < level)) {
            return;
        }
        this._bargraph_msg = msg;
        this._bargraph_min = value_min;
        this._bargraph_max = value_max;
        if (this._log) {
            let div = document.createElement("div");
            div.className = "progress";
            div.textContent = msg + ": ";
            this._progress_bar = document.createElement("progress");
            this._progress_bar.value = 0;
            this._progress_bar.max = 100;
            div.appendChild(this._progress_bar);
            this._log.appendChild(div);
        }
    }

    bargraph_update(value = 0, percent = null) {
        if ((! this._bargraph_msg)) {
            return;
        }
        if (percent === null) {
            if ((this._bargraph_max - this._bargraph_min) > 0) {
                percent = Math.floor((100 * (value - this._bargraph_min)) / (this._bargraph_max - this._bargraph_min));
            } else {
                percent = 0;
            }
        }
        if (percent > 100) {
            percent = 100;
        }
        if (this._progress_bar) {
            this._progress_bar.value = percent;
        }
    }

    bargraph_done() {
        if (this._bargraph_msg) {
            if (this._progress_bar) {
                this._progress_bar.value = 100;
            }

            let elapsed = (Date.now() - this._start_time) / 1000.0;
            this.info(`${this._bargraph_msg}: done in ${elapsed}s`);
            this._bargraph_msg = null;
            this._progress_bar = null;
        }
    }

    set_verbose(verbose) {
        this._verbose = verbose;
    }
}
