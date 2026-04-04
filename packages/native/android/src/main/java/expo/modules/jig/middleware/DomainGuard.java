package expo.modules.jig.middleware;

import expo.modules.jig.JigErrors;
import expo.modules.jig.JigException;
import expo.modules.jig.JigMiddleware;
import expo.modules.jig.JigSessionContext;

import org.json.JSONObject;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** For *.enable / *.disable methods, checks that the domain is supported. */
public class DomainGuard implements JigMiddleware {
    private final Set<String> supportedDomains;

    public DomainGuard(List<String> supportedDomains) {
        this.supportedDomains = new HashSet<>(supportedDomains);
    }

    @Override
    public void validate(String method, JSONObject params,
                         JigSessionContext context) throws JigException {
        if (method.endsWith(".enable") || method.endsWith(".disable")) {
            String domain = method.substring(0, method.indexOf('.'));
            if (!supportedDomains.contains(domain)) {
                throw JigErrors.unsupportedDomain(domain);
            }
        }
    }
}
