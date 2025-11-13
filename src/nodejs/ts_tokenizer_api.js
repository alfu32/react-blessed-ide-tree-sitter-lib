const addon = require('bindings')('tokenizer_addon');
module.exports = {
    SourceCodeParser: addon.SourceCodeParser,
    getLanguages: addon.getLanguages
};
