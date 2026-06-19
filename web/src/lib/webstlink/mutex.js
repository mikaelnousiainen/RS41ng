/* mutex.js
 * Mutex/Condition Variable for serializing access to a shared resource
 *
 * Copyright Devan Lai 2017
 *
 */

export default class Mutex {
    constructor() {
        this.queue = [];
        this.locked = false;
        this.destroyed = false;
    }

    async lock() {
        if (this.destroyed) {
            throw new Error("Mutex is no longer available");
        }

        if (this.locked) {
            let promise = new Promise((resolve, reject) => {
                this.queue.push({ "resolve": resolve, "reject": reject });
            });
            await promise;
        } else {
            this.locked = true;
        }
    }

    unlock() {
        // Signal the first waiting task that they can acquire the mutex
        if (this.locked) {
            if (this.queue.length > 0) {
                this.queue.shift().resolve();
            } else {
                this.locked = false;
            }
        } else {
            throw new Error("Mutex was already unlocked");
        }
    }

    destroy() {
        // Signal all waiting tasks that the resource protected by this
        // mutex is no longer accessible
        for (let promise of this.queue) {
            promise.reject();
        }

        this.locked = true;
        this.destroyed = true;
        this.queue = [];
    }
}
