
/*
 * Copyright (c) 2006-2018 RT-Thread Development Team. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "mbedtls/certs.h"

const char mbedtls_root_certificate[] =
    "-----BEGIN CERTIFICATE-----\r\n" \
    "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n" \
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n" \
    "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n" \
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n" \
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n" \
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n" \
    "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n" \
    "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n" \
    "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n" \
    "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n" \
    "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n" \
    "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n" \
    "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n" \
    "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n" \
    "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n" \
    "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n" \
    "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n" \
    "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n" \
    "MrY=\r\n" \
    "-----END CERTIFICATE-----\r\n" \
    "-----BEGIN CERTIFICATE-----\r\n" \
    "MIIFxjCCBK6gAwIBAgIQDwa7CTBhSCZ/yhtxwduAfTANBgkqhkiG9w0BAQsFADBh\r\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n" \
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n" \
    "MjAeFw0yMjEyMTUwMDAwMDBaFw0zMjEyMTQyMzU5NTlaMFsxCzAJBgNVBAYTAlVT\r\n" \
    "MRcwFQYDVQQKEw5EaWdpQ2VydCwgSW5jLjEzMDEGA1UEAxMqR2VvVHJ1c3QgRzIg\r\n" \
    "VExTIENOIFJTQTQwOTYgU0hBMjU2IDIwMjIgQ0ExMIICIjANBgkqhkiG9w0BAQEF\r\n" \
    "AAOCAg8AMIICCgKCAgEAn1dxOo4OzYa4O1EJd0nhFI5QB/kXAJTHRI6C1J2Gz86B\r\n" \
    "Ge9+DD8R4vexG7/QnUvV+5887o+G4enlkDwJV1Pehq4i0n+X6VKIPg5cThCx6/o0\r\n" \
    "3bUkLWld7slhi3Hli/MaosZZuytdU1uCzQlGpaLB2TiTZbDImiVaykdfwl8V6AXP\r\n" \
    "0Ab4wIcvPggl4qlwyPsBY6NODbP884BmL1ntdXfzecGst30FnAtm4w+PTo0I1T3F\r\n" \
    "ITYXaNuIJnKonD1xXaN4Ar/rvcpKpntxVyZQ2T1Lb7vlfYISqNF+1ugpTzKnI1z7\r\n" \
    "xV7tSqXd2vs6Csy6yFN+QLWwMnGWuQkvrO1m+F7V85S9ECpbqn9qNtKl8gIQ7JeZ\r\n" \
    "BmBdLroW+4j1PDzBWmSWKB+gvRubVSTuoDR3VCtlI4G/Uh+ObF4UStbhKjr1SNCW\r\n" \
    "JcrB1oF+wWBl8Bvozm4xflyyGDY47KCP/Fn9X5GHDNLQA9R0zqDKRumipp1UXswF\r\n" \
    "uzH0I+gOuqSdZSsYID9XSVq6adcJ3rIMAcTuODcrPrJssOFKGFUbmY/VxbxMgYHI\r\n" \
    "/07Zfdxoa+q0osWpofbMhLPZz9UuJFBCLsjgSckkSU02/4itmLm9e3lt6E/4q9Mc\r\n" \
    "MCaS8+qrLWKG9wQtviC8zGynEXpxjEQE8WyrfeYKyXwZFCPB4rpTw1joJmGFeLUC\r\n" \
    "AwEAAaOCAX4wggF6MBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFEFOjmmd\r\n" \
    "9G4l7HgUHH7XzR2Zz/lrMB8GA1UdIwQYMBaAFE4iVCAYlebjbuYP+vq5Eu0GF485\r\n" \
    "MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\r\n" \
    "dAYIKwYBBQUHAQEEaDBmMCMGCCsGAQUFBzABhhdodHRwOi8vb2NzcC5kaWdpY2Vy\r\n" \
    "dC5jbjA/BggrBgEFBQcwAoYzaHR0cDovL2NhY2VydHMuZGlnaWNlcnQuY24vRGln\r\n" \
    "aUNlcnRHbG9iYWxSb290RzIuY3J0MEAGA1UdHwQ5MDcwNaAzoDGGL2h0dHA6Ly9j\r\n" \
    "cmwuZGlnaWNlcnQuY24vRGlnaUNlcnRHbG9iYWxSb290RzIuY3JsMD0GA1UdIAQ2\r\n" \
    "MDQwCwYJYIZIAYb9bAIBMAcGBWeBDAEBMAgGBmeBDAECATAIBgZngQwBAgIwCAYG\r\n" \
    "Z4EMAQIDMA0GCSqGSIb3DQEBCwUAA4IBAQAkpDVI+nCKnAEEpDfldytHXUYOr2ys\r\n" \
    "aGeboE3d1KAN4Gt+eioRvgNdmDKbVFCYY0C6ErIXIvSGXafsprU2dclka/kmH+9U\r\n" \
    "4q3v5Acx6KwcnEuYLN0QOtrlQ9s3Z4IbIzdYUv8IauXC27a3x99x2WMVxNu1KGJy\r\n" \
    "0r1Z+Xya1adf9/e1LFj1CGdq9HfQ/HFWP2khzxQZ4u62sOHuZdPgtNidFOhQLC+2\r\n" \
    "5cNsPgqsaCrGLbSjzni7VN7NhAbTsLhA1oz41vHHB+q4ZzNlC01elmPBOtupe2HM\r\n" \
    "/lqMA/J/nTxc718THdxpszFllAET1/lFEolnqihpUFPaPjRwU7yZIseS\r\n" \
    "-----END CERTIFICATE-----\r\n" \
    "-----BEGIN CERTIFICATE-----\r\n" \
    "MIIHkjCCBXqgAwIBAgIQBFDP9f4iCylgmDivCMqOtjANBgkqhkiG9w0BAQsFADBb\r\n" \
    "MQswCQYDVQQGEwJVUzEXMBUGA1UEChMORGlnaUNlcnQsIEluYy4xMzAxBgNVBAMT\r\n" \
    "Kkdlb1RydXN0IEcyIFRMUyBDTiBSU0E0MDk2IFNIQTI1NiAyMDIyIENBMTAeFw0y\r\n" \
    "NTEwMjQwMDAwMDBaFw0yNjExMDkyMzU5NTlaMH8xCzAJBgNVBAYTAkNOMRIwEAYD\r\n" \
    "VQQIDAnlub/kuJznnIExEjAQBgNVBAcMCea3seWcs+W4gjEtMCsGA1UECgwk5rex\r\n" \
    "5Zyz5Y2B5pa56J6N5rW356eR5oqA5pyJ6ZmQ5YWs5Y+4MRkwFwYDVQQDExBhcGku\r\n" \
    "dGVuY2xhc3MubmV0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA17yf\r\n" \
    "oStgzOemzOx0PgJ/Z12otN37g+dEoBrPj0u1RTDPTrjCLuF8oNWFNotxieB86O90\r\n" \
    "N/bsRyEoJKv9SDF8r1z+YiLlkauvbYbwL2Mp7UKsqZ9BdG9F2kuxCO+O3jTvPztO\r\n" \
    "5Nf+/GS/TmTesq00TqWNcVz3vqWLKIotSG02irFAfodCTc94Hi+v02eYiDA6XsQC\r\n" \
    "IhuEmUxxZNLLSlpD6x0hUmRC2YCa3mpyd163gWTG8PZPQPdgAxwLkEnlrQPMINEf\r\n" \
    "iXeKJlPlsS+Svqb09qwB7wF3QdGXhPEHu0nNYycf6/6+GvEwZkylU0bED+ZwVJ+B\r\n" \
    "kmFqBkVZibMK22zeIQIDAQABo4IDLDCCAygwHwYDVR0jBBgwFoAUQU6OaZ30biXs\r\n" \
    "eBQcftfNHZnP+WswHQYDVR0OBBYEFN48Nx+blr1Rw0toCgUrSj4lwXhcMBsGA1Ud\r\n" \
    "EQQUMBKCEGFwaS50ZW5jbGFzcy5uZXQwPgYDVR0gBDcwNTAzBgZngQwBAgIwKTAn\r\n" \
    "BggrBgEFBQcCARYbaHR0cDovL3d3dy5kaWdpY2VydC5jb20vQ1BTMA4GA1UdDwEB\r\n" \
    "/wQEAwIFoDATBgNVHSUEDDAKBggrBgEFBQcDATBPBgNVHR8ESDBGMESgQqBAhj5o\r\n" \
    "dHRwOi8vY3JsLmRpZ2ljZXJ0LmNuL0dlb1RydXN0RzJUTFNDTlJTQTQwOTZTSEEy\r\n" \
    "NTYyMDIyQ0ExLmNybDCBgwYIKwYBBQUHAQEEdzB1MCMGCCsGAQUFBzABhhdodHRw\r\n" \
    "Oi8vb2NzcC5kaWdpY2VydC5jbjBOBggrBgEFBQcwAoZCaHR0cDovL2NhY2VydHMu\r\n" \
    "ZGlnaWNlcnQuY24vR2VvVHJ1c3RHMlRMU0NOUlNBNDA5NlNIQTI1NjIwMjJDQTEu\r\n" \
    "Y3J0MAwGA1UdEwEB/wQCMAAwggF9BgorBgEEAdZ5AgQCBIIBbQSCAWkBZwB2ANgJ\r\n" \
    "VTuUT3r/yBYZb5RPhauw+Pxeh1UmDxXRLnK7RUsUAAABmhTiv20AAAQDAEcwRQIh\r\n" \
    "AKrkXQPypLxj27/XW3CXJo3sqFZcnd+xW9cOyFV2ltSfAiBuLGgBi+6YXD3BgzdG\r\n" \
    "PGZG9CpKJmoZwBNXIiTlHWdb1QB1AMIxfldFGaNF7n843rKQQevHwiFaIr9/1bWt\r\n" \
    "dprZDlLNAAABmhTiv2cAAAQDAEYwRAIgJOxGBdOXHY9modKivFlLtsudce4iWJWG\r\n" \
    "/L63YgKKsqwCIBTQ8VXeIJE20XGTKjOvJBtayugMvKGC8bnIRPiukF6qAHYAlE5D\r\n" \
    "h/rswe+B8xkkJqgYZQHH0184AgE/cmd9VTcuGdgAAAGaFOK/egAABAMARzBFAiA5\r\n" \
    "ZfLbDa2Wpl0RAmiJHL7XaDUZ5vss3oSjcD3n58fiDAIhAIs8spLqUvMDpxoIac0F\r\n" \
    "x1TGWaIQqF558Q4i5gdMF02PMA0GCSqGSIb3DQEBCwUAA4ICAQBqAqSxtY1kS0Go\r\n" \
    "hQ8x+XGZ6wrbLWpuLfG5O2Zj/Thy5iJd1/ZVjKdl1tq4TA74JL/zOn7i54ZhOrYv\r\n" \
    "c5MSv5fcV8OoPLnp9YMQiaUZQz7IxqSGhbdxWihHj8rDiQtdPcRo4HIgIlJdOJES\r\n" \
    "UbYQKY2JLAKZWw5q4/23muAxo7Y110m7Ns/6GtHZAK7OBHFtAPIwH91ZMt3wynXz\r\n" \
    "Q6r33V/VT+K3ZGKGMR7MUWUpK2QbxkWbfV0FxOPMuwsBHBsCYCQKZG4JSBwU51LC\r\n" \
    "IGWz7I0exGBMbPFtWWNe9M3Q+EpeHp6qIqisrvq/VTvt4mwgVrC2iwznF/jmm/Ja\r\n" \
    "3IKJqYBQn7aZiKSLtc8Erkwp68U1cTWmMk3nKFnlO3aX5EAdLu59JflRptlgAAUq\r\n" \
    "ULHwUg5s8yRNDszk9MJIk1qGqAQlY0ZbPv7zyBTwKpgsQPUI9VpMjoi9ClcUwrMM\r\n" \
    "1HOcgFwFX40tn05s5U6X+zTgFOeadV/jOX/SI8q/55pi81JkeWfZejPdJiEpG0mP\r\n" \
    "Qu1Eg1cxRZCR+Yfy6FCwLFVB0qUNRIxeutQaqjt79zS64Ku2B5U4O+Vp2RUmCPHD\r\n" \
    "xhSFObvoYrA5tf7m8XdVwB+OmuXRTQ3byOY9K4AB9DzEeffb8f1Xi3cyr0NY6vYB\r\n" \
    "EIjXeS/yNN4aD+VUQlWpXffzeoCnQQ==\r\n" \
    "-----END CERTIFICATE-----\r\n" \
    ;

const size_t mbedtls_root_certificate_len = sizeof(mbedtls_root_certificate);

