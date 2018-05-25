# Sphinx extension that builds Contiki-NG documentation and copies it over to
# the sphinx build dir
import subprocess
from sphinx.util import logging
logger = logging.getLogger(__name__)


api_doc_defaults = {
    'doxygen_src_dir': '../doxygen',
    'doxygen_out_dir': 'html',
    'doxygen_suppress_out': True,
    'doxygen_build': True,
}


def api_doc_build(app, exception):
    if exception is not None:
        logger.warning('%s exiting without building' % (__name__,))
        return

    if app.config.api_doc_doxygen_build:
        make_invocation_suffix = ('> /dev/null'
                                  if app.config.api_doc_doxygen_suppress_out
                                  else '')
        make_invocation = ' '.join(('make -C',
                                    app.config.api_doc_doxygen_src_dir,
                                    make_invocation_suffix))

        logger.info('%s building API docs from "%s"'
                     % (__name__, app.config.api_doc_doxygen_src_dir))
        logger.info('%s executing "%s"'
                     % (__name__, make_invocation))

        subprocess.call(make_invocation, shell=True)

        api_doc_build_dir = "/".join((app.config.api_doc_doxygen_src_dir,
                                      app.config.api_doc_doxygen_out_dir))
        api_doc_static_api_dir = "/".join((app.outdir, '_api'))

        logger.info('%s moving "%s" to "%s"'
                     % (__name__, api_doc_build_dir, api_doc_static_api_dir))
        subprocess.call(' '.join(('mv', api_doc_build_dir,
                                  api_doc_static_api_dir)),
                        shell=True)

        # Fundamentally a workaround: Readthedocs doxygen build plain refulses
        # to build the same html/*.js files as local builds do. So we ship them
        # and we copy them over to the output dir by force, till readthedocs
        # starts behaving, hopefully in the near future
        subprocess.call(' '.join(('cp js/dynsections.js',
                                  api_doc_static_api_dir)),
                        shell=True)


def setup(app):
    for k, v in api_doc_defaults.items():
        config_val = 'api_doc_' + k
        logger.debug('Add config value %s: %s' %(config_val, v))
        app.add_config_value(config_val, v, '')

    # We will build and copy the docs after the end of the sphinx build, and.
    # only if it has been successful. Regsiter for the build-finished event.
    app.connect('build-finished', api_doc_build)

    logger.info('%s initialised' % (__name__,))
