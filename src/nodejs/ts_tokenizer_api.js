const addon = require('bindings')('tokenizer_addon');
module.exports = {
    ParserManager: addon.ParserManager,
    getLanguages: addon.getLanguages
};
