#include <stdio.h>
#include <locale.h>
#include <libcc.h>
#include <libcc/widgets/xml.h>

const tchar_t *xml_test = _T("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>")\
    _T("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">")\
    _T("<Configuration status = \"debug\" strict=true name=\"XMLConfigTest\" packages=\"org.apache.logging.log4j.test\">")\
    _T("    <Properties>")\
    _T("        <Property name=\"filename\" name=\"filename1\" name=\"filename2\" name=\"filename3\">target/中文.log</Property>")\
    _T("    </Properties>")\
    _T("    <Filter type=\"ThresholdFilter\" level=\"trace\" />")\
    _T("    <Appenders>")\
    _T("        <Appender type=\"Console\" name=\"STDOUT\">")\
    _T("            <Layout type=\"PatternLayout\" pattern=\"%m MDC%X%n\" />")\
    _T("            <Filters>")\
    _T("                <Filter type=\"MarkerFilter\" marker=\"FLOW\" onMatch=\"DENY\" onMismatch=\"NEUTRAL\" />")\
    _T("                <Filter type=\"MarkerFilter\" marker=\"EXCEPTION\" onMatch=\"DENY\" onMismatch=\"ACCEPT\" />")\
    _T("            </Filters>")\
    _T("        </Appender>")\
    _T("        <Appender type=\"Console\" name=\"FLOW\">")\
    _T("            <Layout type=\"PatternLayout\" pattern=\"%C{1}.%M %m %ex%n\" />")\
    _T("            <!-- class and line number -->")\
    _T("            <Filters>")\
    _T("                <Filter type=\"MarkerFilter\" marker=\"FLOW\" onMatch=\"ACCEPT\" onMismatch=\"NEUTRAL\" />")\
    _T("                <Filter type=\"MarkerFilter\" marker=\"EXCEPTION\" onMatch=\"ACCEPT\" onMismatch=\"DENY\" />")\
    _T("            </Filters>")\
    _T("        </Appender>")\
    _T("        <Appender type=\"File\" name=\"File\" fileName=\"${filename}\">")\
    _T("            <Layout type=\"PatternLayout\">")\
    _T("                <Pattern><span color=\"#f00\">ABCD</span><![CDATA[%d %p %C{1.} [%t] %m%n]]><![CDATA[<html>&ltbody&gt&lt/body&gt</html>]]></Pattern>")\
    _T("            </Layout>")\
    _T("        </Appender>")\
    _T("        <Appender type=\"List\" name=\"List\"></Appender>")\
    _T("    </Appenders>")\
    _T("    <Loggers>")\
    _T("        <Logger name=\"org.apache.logging.log4j.test1\" level=\"debug\" additivity=\"false\">")\
    _T("            <Filter type=\"ThreadContextMapFilter\">")\
    _T("                <KeyValuePair key=\"test\" value=\"123\" />")\
    _T("            </Filter>")\
    _T("            <AppenderRef ref=\"STDOUT\" />")\
    _T("        </Logger>")\
    _T("        <Logger name=\"org.apache.logging.log4j.test2\" level=\"debug\" additivity=\"false\">")\
    _T("            <AppenderRef ref=\"File\" />")\
    _T("        </Logger>")\
    _T("        <Root level=\"trace\">")\
    _T("            <AppenderRef ref=\"List\" />")\
    _T("        </Root>")\
    _T("    </Loggers>")\
    _T("</Configuration>");

int _tmain (int argc, tchar_t * const argv[]) {
    
    _cc_xml_t* xml;

    if (argc < 2) {
        xml = _cc_xml_parse(xml_test,-1);//
    } else {
        xml = _cc_xml_from_file(argv[1]);
    }
    
    if (xml) {
        _cc_xml_t *item = _cc_xml_element_find(xml,_T("Configuration/Properties/Property"));
        if (item) {
            _cc_xml_element_set_attr(item, _T("name"), _T("%s-%d"), _T("test"), 100);
            _cc_xml_element_set_attr(item, _T("name2"), _T("%s-%d"), _T("test"), 200);
            const tchar_t *name = _cc_xml_element_attr_find(item, _T("name"));
            item = _cc_xml_element_first_child(item);
            _tprintf(_T("context:%s\nattr name=%s\n"), item->element.uni_context.text, name);
            
        }
        /*
        _cc_xml_t *item = _cc_xml_element_find(xml,_T("Configuration/Loggers/Logger/"));
        if (item) {
            item = _cc_xml_element_first_child(item);
            while (item) {
                if (item->name) {
                    _tprintf(_T("name:%s\n"), item->name);
                }
                
                item = _cc_xml_element_next_child(item);
            }
        }*/

        {
        _cc_buf_t buf 
        _cc_dump_xml(xml, buf);
        //fflush(stdout);
        //fwrite(buf->bytes, sizeof(byte_t), buf->length, stdout);
        _tprintf(_T("%s"), (tchar_t*)buf->bytes);
        _cc_free_buf(&buf);
        }
        _cc_free_xml(xml);
    } else {
        _tprintf(_T("XML parse Fail:%s\n"), _cc_xml_error());
    }
    
    system("pause");
    return 0;
}
